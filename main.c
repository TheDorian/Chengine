//
//  main.c
//  Chengine
//
//  Created by Henning Sperr on 6/7/12.
//  Copyright 2012 Henning Sperr. All rights reserved.
//

#include "Chengine.h"



int main (int argc, const char * argv[])
{
    
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    
    Game newGame;
    ChError hr;
    initTable(1048576*16);
    
    newGame.isRunning=1;
    newGame.aiPlayer[WHITE].depth=0;
    newGame.aiPlayer[WHITE].timelimit=400000;
    newGame.aiPlayer[WHITE].isAi=1;
    
    newGame.aiPlayer[BLACK].depth=0;
    newGame.aiPlayer[BLACK].timelimit=300000;
    newGame.aiPlayer[BLACK].isAi=0;
    
    if((hr=initBoard(&newGame.board))){
        printf("After init board:");
        printError(hr);
    }

    readFENString(&newGame.board, "7k/5K2/5P1p/3p4/6P1/3p4/8/8 w - - 0 1");
    //printf("Evaluate: %d\n",evaluate(&newGame.board));
    printBoardE(&newGame.board);
    while(newGame.isRunning){
            
        if(newGame.aiPlayer[newGame.board.colorToPlay].isAi){
            //hr=doAiMove(&newGame.board, &newGame.aiPlayer[newGame.board.colorToPlay]);
            hr=doAi(&newGame.board);
            switch (hr) {
                case ChError_OK:
                    break;
                case ChError_CheckMate:
                    if(newGame.board.colorToPlay==WHITE){
                        printf("0-1 {Black mates}\n");
                        newGame.aiPlayer[BLACK].isAi=0;
                        newGame.aiPlayer[WHITE].isAi=0;
                    }else{
                        printf("1-0 {White mates}\n");
                        newGame.aiPlayer[BLACK].isAi=0;
                        newGame.aiPlayer[WHITE].isAi=0;
                    }
                    break;
                case ChError_StaleMate:
                        printf("1/2-1/2 {Stalemate}\n");
                        newGame.aiPlayer[BLACK].isAi=0;
                        newGame.aiPlayer[WHITE].isAi=0;
                    break;
                case ChError_RepetitionDraw:
                    printf("1/2-1/2 {Repetition Draw}\n");
                    newGame.aiPlayer[BLACK].isAi=0;
                    newGame.aiPlayer[WHITE].isAi=0;
                    break;

                default:
                     printf("After doAiMove:");
                    printError(hr);
                    break;
            }
        }else{
            checkForInput(&newGame); 
        }
    }

    
    //TESTING
  
     /*
    printf("Perfting\n");
    readFENString(&newGame.board, "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - - 0 0");
   // readFENString(&newGame.board, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0");
    printBoardE(&newGame.board);
    perft(&newGame.board,1);
    perft(&newGame.board,2);
    perft(&newGame.board,3);
    perft(&newGame.board,4);
    perft(&newGame.board,5);
    perft(&newGame.board,6);
    
   // 

    printf("Perfting 1 2 3 4\n");
    perft_hash(&newGame.board,1);
    perft_hash(&newGame.board,2);
    perft_hash(&newGame.board,3);
    perft_hash(&newGame.board,4);
    printf("Perfting 5 6 7 8\n");

    perft_hash(&newGame.board,5);
    perft_hash(&newGame.board,5);

    perft_hash(&newGame.board,6);
   // perft_hash(&newGame.board,7);
   // perft_hash(&newGame.board,8);
    
    */
    freeTable();
    return 0;
}



