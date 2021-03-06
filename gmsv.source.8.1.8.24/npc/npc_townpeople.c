#include "version.h"
#include "object.h"
#include "char_base.h"
#include "char.h"
#include "util.h"
#include "handletime.h"
#include "anim_tbl.h"
#include "npc_door.h"
#include "lssproto_serv.h"
#include "npcutil.h"


/*
 *  まちのひとびと  by nakamura
 *  タイプ  �uTownPeople
 *    しかけられたときにNPCARGUMENTの  字  を  しかけてきた人に  す�e
 *
 *  また�bnpcargに こんにちは,今  はてんきがいいですね
 *  のように  角コンマをデリミタとして  数のメーセージを
 *  書くことができ�bその場合ランダムでその  からしゃべる�e
 *  npcgen.perl では�b MANである�e MSGとほとんど恭なじだなあ
 *
 */

/*
 *   しかけられたときはNPCARGUMENTをそのまま  す�e
 */
void NPC_TownPeopleTalked( int index, int talker, char *msg, int color )
{
	char arg1[NPC_UTIL_GETARGSTR_BUFSIZE], token[NPC_UTIL_GETARGSTR_LINEMAX];
	char *arg;
    int i, tokennum;

    /* 3グリッド以  の場合だけ返答する */

	if( CHAR_getInt(talker,CHAR_WHICHTYPE) == CHAR_TYPEPLAYER 
        && NPC_Util_charIsInFrontOfChar( talker, index, 3 ) ){

        arg = NPC_Util_GetArgStr( index, arg1, sizeof( arg1));

        tokennum = 1;
        /* コンマで区切られたトークンが何こあるか数える */
        for( i=0;arg[i]!='\0';i++ ){
            if( arg[i] == ',' ) tokennum++;
        }

        /* ランダムでどれを喋るか決めて�bそのトークンを取りだす */
        getStringFromIndexWithDelim( arg,",",
                                     rand()%tokennum+1,token, sizeof(token));

        CHAR_talkToCli( talker, index, token, CHAR_COLORWHITE );
    }
}

/*
 * 初期化する�e
 */
BOOL NPC_TownPeopleInit( int meindex )
{

    //CHAR_setInt( meindex , CHAR_HP , 0 );
    //CHAR_setInt( meindex , CHAR_MP , 0 );
    //CHAR_setInt( meindex , CHAR_MAXMP , 0 );
    //CHAR_setInt( meindex , CHAR_STR , 0 );
    //CHAR_setInt( meindex , CHAR_TOUGH, 0 );
    //CHAR_setInt( meindex , CHAR_LV , 0 );

    CHAR_setInt( meindex , CHAR_WHICHTYPE , CHAR_TYPETOWNPEOPLE );
    //CHAR_setFlg( meindex , CHAR_ISOVERED , 1 );
    //CHAR_setFlg( meindex , CHAR_ISATTACKED , 0 );  /*   撃されないよん */
    
    return TRUE;
}
