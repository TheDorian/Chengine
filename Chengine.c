//
//  Chengine.c
//  Chengine
//
//  Created by Henning Sperr on 6/26/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include "Chengine.h"


ChError addToMoveList(MoveList* moveList, Move* move){
    if(moveList->nextFree==moveList->alloc){
        int alloc = max(2048, moveList->alloc * 2);
        Move* newPtr = (Move*) realloc(moveList->array, alloc * sizeof(Move));
        if(newPtr==NULL)
            return ChError_Resources;
        moveList->array=newPtr;
        moveList->alloc=alloc;
    }
    moveList->array[moveList->nextFree]=*move;
    moveList->nextFree++;
    
    return ChError_OK;
}
void freeMoveList(MoveList* moveList){
    moveList->nextFree=0;
    moveList->alloc=0;
    if(moveList->array)
        free(moveList->array);
    moveList->array=NULL;
}

void printMoveList(MoveList* moveList){
    char charMove[6];
    for(int i=0;i<moveList->nextFree;i++){
        moveToChar(&moveList->array[i],charMove);
        
        printf("%d. %s - ",i,charMove);
    }
}

void printMoveListFromOffset(MoveList* moveList, int fromOffset){
    char charMove[6];
    for(int i=fromOffset;i<moveList->nextFree;i++){
        moveToChar(&moveList->array[i],charMove);
        
        printf("%d. %s - ",i,charMove);
    }
    printf("\n");
}

void printError(ChError hr){
    printf("ERROR: ");
    switch(hr){
        case ChError_OK:
            printf("Okay\n");
            break;
        case ChError_Arguments:
            printf("Arguments\n");
            break;
        case ChError_BrokenFenString:
            printf("No last move\n");
            break;
        case ChError_Resources:
            printf("Resources\n");
            break;
        case ChError_NotInTable:
             printf("Value not in table\n");
            break;
        case ChError_DepthToLow:
             printf("Depth was too low\n");
            break;
        case ChError_IllegalMove:
             printf("Illegal move\n");
            break;
        case ChError_CheckMate:
             printf("Check mate!\n");
            break;
        case ChError_StaleMate:
             printf("Stale mate!\n");
            break;
        default:
            printf("No Error: %d\n",hr);
    }
}