//
//  NegaScout.c
//  Chengine
//
//  Created by Henning Sperr on 7/1/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include "NegaScout.h"

#include "assert.h"

#define MATE_SCORE 300000
#define INIT_ALPHA -1000000
#define INIT_BETA  1000000
#define DRAW_SCORE 0

int centerWhiteValue[]={1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    3,4,4,5,5,4,4,3,0,0,0,0,0,0,0,0,
    3,4,4,4,4,4,4,3,0,0,0,0,0,0,0,0,
    2,2,4,4,4,4,2,2,0,0,0,0,0,0,0,0,
    2,2,3,4,4,3,2,2,0,0,0,0,0,0,0,0,
    2,2,3,3,3,3,2,2,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};
int centerBlackValue[]={1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
    2,2,3,3,3,3,2,2,0,0,0,0,0,0,0,0,
    2,2,3,4,4,3,2,2,0,0,0,0,0,0,0,0,
    2,2,3,4,4,3,2,2,0,0,0,0,0,0,0,0,
    2,2,4,4,4,4,2,2,0,0,0,0,0,0,0,0,
    3,4,4,5,5,4,4,3,0,0,0,0,0,0,0,0,
    3,4,4,4,4,4,4,3,0,0,0,0,0,0,0,0};
/*
 
 {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0}
 */

int cutOff[10];

static int FUTILITY_VALS[] = {0, 120, 120, 310, 310, 400}; // Shamelessly stolen from Crafty



typedef struct AiStatistics{
    int allMovesCalculated;
    int movesPerIterationCalculated;
    int quietNodes;
    int tableLookUpsFound;
    int cutOffs;
    MoveList* list;
    Move bestMove;
    int bestMoveScore;
}AiStatistics;

const char useAlphaBeta=1;
const char useQuiescent=1;
const char useHashTables=1;
const char usePVS=1;
const char useNullMoves=1;
const char useAspiration=1;
const char useOpeningTable=0;
const char useComplexEvaluation=1;
const char useLateMoveReduction=0;
const char useFutilityPruning=0;

const int ASPIRATION_SIZE=30;
const int MAX_DEPTH=100;
const char NULL_DEPTH=3;

const int TIME_CHECK_INTERVAL=40000;
int nextTimeCheck=1000;
long startTime=0;
float timeForThisMove=0;
int stopSearch=0;

static int quiscent(ChessBoard* board,int alpha, int beta, SearchInformation* info){
    info->currentDepth++;
    int score=0;    
    
    
    if(useComplexEvaluation)
        score=EvaluateComplex(board);
    else
        score=evaluate(board);
    
    MoveList* list=info->list;
    if(score>alpha)
    {
        if(score>=beta)
            return beta;
        
        alpha=score;
    }
    
    int offset=list->nextFree;
    
    ChError hr;
    hr=generateMoves(board, board->colorToPlay,list, info);
    switch(hr){
        case ChError_OK:
            break;
        case ChError_StaleMate:
            list->nextFree=offset;
            info->currentDepth--;
            return 0;
            break;
        case ChError_CheckMate:
            list->nextFree=offset;
            return -MATE_SCORE;
            break;
        default:
            printError(hr);
            break;
    }
    
    
    for(int i=offset;i<list->nextFree&&!stopSearch;i++){        
        info->quietNodes++;
        Move* move = {0};
        int maxIndex=0;
        int bestScoredMove=-100000;
        
        for(int j=offset;j<info->list->nextFree;j++){
            if(info->list->array[j].score>bestScoredMove&&board->tiles[info->list->array[j].to]){
                bestScoredMove=info->list->array[j].score;
                maxIndex=j;
                
            }
            
        }
        
        if(bestScoredMove==-100000)
            break;
        
        move=&info->list->array[maxIndex];
        move->score=-100000;
        
        
        doMove(board,move);
        score=-quiscent(board,-beta,-alpha,info);
        undoLastMove(board);
        
        if(score>alpha){
            alpha=score;
            if(score>=beta){
                list->nextFree=offset;
                info->currentDepth--;
                return beta;
            }
        }
        
    }
    
    list->nextFree=offset;
    info->currentDepth--;
    return alpha;
    
}
static int NegaScoutRoot(int alpha, int beta, int depth, int allowNull, SearchInformation* info){
    int myGlobOffset = info->list->nextFree;
    info->currentDepth=depth;
    int pvsBeta=beta;
    int localAlpha=0;
    ChError hr=ChError_OK;
    Bound bound=HASH_BETA;
    int threat=0;
    //ADD TIMING HERE
    nextTimeCheck--;
    if(nextTimeCheck==0){
        nextTimeCheck=TIME_CHECK_INTERVAL;
        if((float)(clock()-startTime)/CLOCKS_PER_SEC>=timeForThisMove){
            stopSearch=1;
            return 0;
        }
    }
    //ADD REPETITION DRAW AND 50 MOVE RULE HERE
    
    if(probeRepetitionTable(&info->board->zobrist)>=3||info->board->repetitionMoves>=50){
        return 0;
    }
    
    if(depth<=0){
        info->movesPerIterationCalculated++;
        
        if(useQuiescent)
            return quiscent(info->board, alpha, beta, info);
        else{
            if(useComplexEvaluation)
                return EvaluateComplex(info->board);
            else
                return evaluate(info->board);
        }
    }
    
    //PROBE HASHTABLE HERE
    Move hashMove={0};
    if(useHashTables){
        if((hr=probe(info->board->zobrist, depth,&alpha,&beta, &localAlpha,&hashMove))){
            //not in table
        }else{
            info->hashExactCutoffs++;//exact hit
            
            //return localAlpha;
        }
        if(hashMove.value==0&&depth!=info->globalDepth&&alpha!=beta-1&&depth>3){
            int globaleDepth=info->globalDepth;
            info->globalDepth=depth-2;
            Move bestMove=info->bestMove;
            info->bestMove.value=0;
            info->internalDeepening=1;
            //internal iterative deepening
            NegaScoutRoot(INIT_ALPHA, INIT_BETA, depth-2, 0, info);
            info->internalDeepening=0;
            hashMove=info->bestMove;
            info->bestMove=bestMove;
            info->globalDepth=globaleDepth;
            info->currentDepth=depth;
#ifdef DEBUG       
            //assert(isLegal(info->board, &hashMove));
#endif 
            
        }
        
        //try hashmove first
        if(hashMove.value!=0&&isLegal(info->board, &hashMove)){
            Move* move = &hashMove;
            doMove(info->board, move);
            
            
            if(usePVS){
                localAlpha=-NegaScoutRoot(-pvsBeta, -alpha, depth-1,useNullMoves, info);
                if(localAlpha>alpha&&localAlpha<beta){
                    info->pvsFail++;
                    localAlpha=-NegaScoutRoot(-beta, -alpha, depth-1,useNullMoves, info);
                }
            }else{
                localAlpha=-NegaScoutRoot(-beta, -alpha, depth-1,useNullMoves, info); 
            }
            
            undoLastMove(info->board);
            info->currentDepth=depth;
            if(alpha<localAlpha){
                bound=HASH_EXACT;
                alpha=localAlpha;
                if(((alpha>=beta||alpha>=MATE_SCORE)&&useAlphaBeta)){
                    info->hashMoveCutOffs++;
                    info->list->nextFree=myGlobOffset;
                    addKeyToTable(info->board->zobrist, depth, alpha, bound,hashMove);
                    return alpha;
                    
                }
                
            }
            pvsBeta=alpha+1;
            localAlpha=INIT_ALPHA;
        }
    }
    
    //int eval=0;
    //probeEvalTable(&info->board->zobrist, &eval);
    int isInCheck=isCheck(info->board,info->board->colorToPlay);
    if(allowNull&& //no two nullmoves in a row
       alpha!=beta-1&& //not a pv search     
       // eval>beta&&
       depth>NULL_DEPTH)
    {
        if(!isInCheck)
        {
            info->board->colorToPlay=info->board->colorToPlay==WHITE?BLACK:WHITE;
            switchColorZobrist(&info->board->zobrist);
            setEnPassantZobrist(&info->board->zobrist, info->board->enPassantSquare,-5);
            int temp=info->board->enPassantSquare;
            info->board->enPassantSquare=-5;
            
            int eval = 0;
            eval=-NegaScoutRoot(-beta, -beta+1, depth-1-2, 0, info);
            info->currentDepth=depth;
            info->board->colorToPlay=info->board->colorToPlay==WHITE?BLACK:WHITE;
            switchColorZobrist(&info->board->zobrist);
            setEnPassantZobrist(&info->board->zobrist, temp,-5);
            info->board->enPassantSquare=temp;
            
            if(eval >= beta){
                info->nullCutOffs++;
                return eval;
            }
            if(eval <-MATE_SCORE){
                //printf("Happens");
                threat=1;
            }
        }
    }
    
    hr=generateMoves(info->board, info->board->colorToPlay, info->list,info);
    switch (hr){
        case ChError_OK:
            break;
        case ChError_CheckMate:
            //we know that we found no moves
            alpha=-MATE_SCORE-depth; //evaluate situation as bad
            break;
        case ChError_StaleMate:
            alpha=DRAW_SCORE;
            break;
        default:
            printError(hr);
            break;
    }
    
    Move localBestMove={0};
    for(int moveNr=myGlobOffset; moveNr<info->list->nextFree&&!stopSearch;moveNr++){
        Move* move = {0};
        int maxIndex=0;
        int bestScoredMove=-10000;
        //char moveChar[6];
        for(int i=myGlobOffset;i<info->list->nextFree;i++){
            if(info->list->array[i].score>bestScoredMove){
                bestScoredMove=info->list->array[i].score;
                maxIndex=i;
            }
            // moveToChar(&info->list->array[i], moveChar);
            // printf("Move %s score %d\n",moveChar,info->list->array[i].score);
        }
        //printf("\n");
        move=&info->list->array[maxIndex];
        move->score=-100000;
        
        //futility pruning
        if(useFutilityPruning&&moveNr>=myGlobOffset+1&&depth<5&&depth>1&&info->globalDepth>=10&&!isInCheck){
            int gain=0;
            int eval =EvaluateComplex(info->board);
            if(info->board->tiles[move->to]){
                gain+=getPieceScore(info->board->tiles[move->to]->piece)/2;
            }
            if(move->moveType==PROMOTION){
                gain+=900;
            }
            if((eval+gain+FUTILITY_VALS[depth])<=alpha){
                if(eval+gain>localAlpha)
                    localAlpha=eval+gain;
                continue;
            }
        }
        
        int isCapture=info->board->tiles[move->to]!=NULL?1:0;
        if(move->moveType==PROMOTION)threat=1;
        
        doMove(info->board, move);
        
        if(usePVS){
            //late move reduction
            if(useLateMoveReduction&&moveNr>myGlobOffset+5&& //more than three moves searched
               !isInCheck&&//not in check
               depth>=3&& 
               !threat&&
               !isCapture)//no capture
            {
                localAlpha=-NegaScoutRoot(-pvsBeta, -alpha, depth-2,useNullMoves, info);
            }else{
                localAlpha=-NegaScoutRoot(-pvsBeta, -alpha, depth-1,useNullMoves, info);
            }
            if(localAlpha>alpha&&localAlpha<beta&&moveNr>myGlobOffset){
                info->pvsFail++;
                localAlpha=-NegaScoutRoot(-beta, -alpha, depth-1,useNullMoves, info);
            }
        }else{
            localAlpha=-NegaScoutRoot(-beta, -alpha, depth-1,useNullMoves, info); 
        }
        
        undoLastMove(info->board);
        info->currentDepth=depth;
        if(alpha<localAlpha){
            bound=HASH_EXACT;
            alpha=localAlpha;
            localBestMove=*move;
            if(info->globalDepth==depth)
                info->bestMove=*move;
            
            if(((alpha>=beta||alpha>=MATE_SCORE)&&useAlphaBeta)){
                info->cutOffs++;
                bound=HASH_ALPHA;
                
                if(!info->board->tiles[move->to]){
                    info->history[move->from][move->to]+=2<<depth;
                    //noncapture add killer
                    if(info->killerMoves[depth][0].value==move->value){
                        info->killerMoves[depth][1]=info->killerMoves[depth][0];
                        info->killerMoves[depth][0]=*move;
                    }
                }
                break;
            }
            
        }
        pvsBeta=alpha+1;
        localAlpha=INIT_ALPHA;
        
    }
    
    info->list->nextFree=myGlobOffset;
    addKeyToTable(info->board->zobrist, depth, alpha, bound,localBestMove);
    return alpha;
}

void printPV_rec(ChessBoard* board, Move* move, int maxDepth){
    if(maxDepth==0)
        return;
    Move hashMove={0};
    int someval=0;
    char moveChar[6];
    if(move->value!=0&&isLegal(board, move)){
        doMove(board, move);
        moveToChar(move, moveChar);
        printf("%s ",moveChar);
        probe(board->zobrist, 0, &someval, &someval, &someval, &hashMove);
        
        printPV_rec(board,&hashMove,maxDepth-1);
        undoLastMove(board);
    }
}


void printPV(ChessBoard* board, Move* move, int maxDepth){
    printPV_rec(board, move, maxDepth);
    printf("\n");
}

ChError UseOpeningTable(ChessBoard* board, Properties* player){
      char charMove[6];
    char fenString[70];
    getFenString(board, fenString);
    uint64 openingHash=OpeningBookHash(fenString);
    Move openingMove;
    openingMove=openBookAndGetNextMove("/Users/henningsperr/Desktop/Chess/chen/Chengine/Book.bin", &openingHash);
    if(openingMove.from!=openingMove.to){
        //valid move
        //move type
        if(board->tiles[openingMove.from]->piece==king){
            if(openingMove.from-openingMove.to==0x02){
                openingMove.moveType=board->colorToPlay==WHITE?WQUEENCASTLE:BQUEENCASTLE;
            }
            if(openingMove.from-openingMove.to==-0x02){
                openingMove.moveType=board->colorToPlay==WHITE?WKINGCASTLE:BKINGCASTLE;
            }
            
            
        }else if(board->tiles[openingMove.from]->piece==pawn){
            if(openingMove.promote!=0)
                openingMove.moveType=PROMOTION;
            else if(openingMove.from-openingMove.to==0x20||
                    openingMove.from-openingMove.to==-0x20){
                openingMove.moveType=PAWNDOUBLE;
            }else if(openingMove.to==board->enPassantSquare){
                openingMove.moveType=ENPASSANT;
            }
        }    
        moveToChar(&openingMove, charMove);
        printf("move %s\n",charMove);
        
        doMove(board, &openingMove);
        printBoardE(board);
        return ChError_OK;
    }else{
        player->useOpeningTable=0;
    }
    
    return ChError_OK;
}

ChError doAi(ChessBoard* board, Properties* player){
    //print move
    char charMove[6];
    
    if(player->useOpeningTable&&useOpeningTable){
        return UseOpeningTable(board,player);
    }else{
        //NO OPENINGTABLE MOVE FOUND DO NORMAL SEARCH
        clearHashTable();
        int alpha=INIT_ALPHA;
        int beta = INIT_BETA;
        ChError hr=ChError_OK;
        SearchInformation info ={0};
        MoveList list={0};
        
        info.board=board;
        info.list=&list;
        
        hr=generateMoves(board, board->colorToPlay, &list, &info);
        switch (hr){
            case ChError_OK:
                break;
            case ChError_CheckMate:
                return hr;
                break;
            case ChError_StaleMate:
                return hr;
                break;
            default:
                printError(hr);
                break;
        }
        info.list->nextFree=0;
        
        timeForThisMove=max(0.1,(float)player->timelimit/(30*100-(min(10,(board->playedMoves.nextFree/2))*100)));
        startTime=clock();
        stopSearch=0;
        nextTimeCheck=TIME_CHECK_INTERVAL;
        
        long time=clock();
        int depth =1;
        Move bestFinishedMove;
        int overallBestScore;
        for(;!stopSearch&&depth<=player->depth&&depth<MAX_DEPTH;depth+=1){
            info.globalDepth=depth;
            int bestScore=NegaScoutRoot(alpha, beta, depth,0, &info);
            
            if(stopSearch)
                break;
            
            //fell out of aspiration window, search again
            if(useAspiration){
                if(bestScore<=alpha||bestScore>=beta){
                    alpha=INIT_ALPHA;
                    beta=INIT_BETA;
                    //   printf("#Aspiration Window Fail:have to search again\n");
                    depth-=1;//search same depth again
                    continue;
                }else{
                    alpha=bestScore-ASPIRATION_SIZE;
                    beta=bestScore+ASPIRATION_SIZE;
                }
            }else{
                alpha=INIT_ALPHA;
                beta=INIT_BETA;            
            }
            
            /*for(int a=0;a<10;a++){
             printf("%d ",cutOff[a]);
             }
             printf("\n");*/
            
            /***********
             *STATISTICS
             ************/   
            
            //ply score time nodes pv
            printf("%d %d %d %d ",depth,bestScore,(clock()-startTime)/10000,info.movesPerIterationCalculated+info.quietNodes);
            printPV(board,&info.bestMove,depth);
            bestFinishedMove=info.bestMove;
            overallBestScore=bestScore;
            // printf("#In Depth %d best move score %d with move %s out of %d nodes with %d qis\n",depth,info.bestMoveScores[depth],charMove,info.movesPerIterationCalculated,info.quietNodes);
            
            
            info.allMovesCalculated+=info.quietNodes;
            info.allMovesCalculated+=info.movesPerIterationCalculated;
            info.quietNodes=0;
            info.movesPerIterationCalculated=0; 
            
            if((float)(clock()-startTime)/CLOCKS_PER_SEC*2>timeForThisMove ){
                depth++;
                break;
            }
            
        }
        
        
        long rep=probeRepetitionTable(&board->zobrist);
        if((rep>=3||board->repetitionMoves>=50)&&overallBestScore<0){
            hr=ChError_RepetitionDraw;
            return hr;
        }
        
        
        printf("#Found best move %s in %f sec out of %lu nodes having %f nodes/sec.\n",charMove,((float)(clock()-time)/CLOCKS_PER_SEC),info.allMovesCalculated,(info.allMovesCalculated+info.quietNodes)/((float)(clock()-time)/CLOCKS_PER_SEC));
        printf("#This run i had %d tableLookups and %d cutoffs and %d quiet nodes.\n",info.hashExactCutoffs,info.cutOffs,info.quietNodes);
        
        moveToChar(&bestFinishedMove, charMove);
        printf("move %s\n",charMove);
        doMove(board, &bestFinishedMove);
        
        
        printBoardE(board);
        
        
        hr=generateMoves(board, board->colorToPlay, &list,&info);
        switch (hr){
            case ChError_OK:
                break;
            case ChError_CheckMate:
                return hr;
                break;
            case ChError_StaleMate:
                return hr;
                break;
            default:
                printError(hr);
                break;
        }
        
        return hr;
    }
    return ChError_OK;
}


ChError doAiTest(ChessBoard* board, int searchDepth,SearchInformation* info){
    //print move
    
    clearHashTable();
    
    int alpha=INIT_ALPHA;
    int beta = INIT_BETA;
    
    ChError hr=ChError_OK;
    
    MoveList list={0};
    
    info->board=board;
    info->list=&list;
    info->movesPerIterationCalculated=0;
    
    hr=generateMoves(board, board->colorToPlay, &list,info);
    switch (hr){
        case ChError_OK:
            break;
        case ChError_CheckMate:
            return hr;
            break;
        case ChError_StaleMate:
            return hr;
            break;
        default:
            printError(hr);
            break;
    }
    
    list.nextFree=0;
    
    timeForThisMove=100000000; //endless time, tests are only limited by depth
    startTime=clock();
    stopSearch=0;
    nextTimeCheck=TIME_CHECK_INTERVAL;
    
    int depth =1;
    Move bestFinishedMove;
    int overallBestScore;
    for(;!stopSearch&&depth<=searchDepth&&depth<MAX_DEPTH;depth++){
        info->globalDepth=depth;
        
        int bestScore=NegaScoutRoot(alpha, beta, depth,0, info);
        
        if(stopSearch)
            break;
        
        //fell out of aspiration window, search again
        if(useAspiration){
            if(bestScore<=alpha||bestScore>=beta){
                alpha=INIT_ALPHA;
                beta=INIT_BETA;
                //   printf("#Aspiration Window Fail:have to search again\n");
                depth--;//search same depth again
                continue;
            }else{
                alpha=bestScore-ASPIRATION_SIZE;
                beta=bestScore+ASPIRATION_SIZE;
            }
        }else{
            alpha=INIT_ALPHA;
            beta=INIT_BETA;            
        }
        
        info->totalTimeUsed+=(clock()-startTime);
        //ply score time nodes pv
        printf("%d %d %f %d ",depth,bestScore,(float)(clock()-startTime)/CLOCKS_PER_SEC,info->movesPerIterationCalculated+info->quietNodes);
        bestFinishedMove=info->bestMove;
        overallBestScore=bestScore;
        printPV(board, &bestFinishedMove,depth);
        // printf("#In Depth %d best move score %d with move %s out of %d nodes with %d qis\n",depth,info.bestMoveScores[depth],charMove,info.movesPerIterationCalculated,info.quietNodes);
        
        
        info->globalQuietNodes+=info->quietNodes;
        info->allMovesCalculated+=info->movesPerIterationCalculated;
        info->quietNodes=0;
        info->movesPerIterationCalculated=0; 
        
        
    }
    return hr;
    
}