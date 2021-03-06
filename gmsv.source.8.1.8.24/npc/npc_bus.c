#include "version.h"
#include <string.h>
#include "char.h"
#include "object.h"
#include "char_base.h"
#include "npcutil.h"
#include "lssproto_serv.h"
#include "npc_bus.h"
#include "handletime.h"

/* 
 * バスみたいな事をするNPC
 */
 
enum {
	NPC_WORK_ROUTETOX = CHAR_NPCWORKINT1,		/* どこへ！  座   */
	NPC_WORK_ROUTETOY = CHAR_NPCWORKINT2,		/* どこへ！  座   */
	NPC_WORK_ROUTEPOINT = CHAR_NPCWORKINT3,		/* 今何    か */
	NPC_WORK_ROUNDTRIP = CHAR_NPCWORKINT4,		/* 行きか帰りか  ０�u行き １�u帰り  */
	NPC_WORK_MODE = CHAR_NPCWORKINT5,
	NPC_WORK_CURRENTROUTE = CHAR_NPCWORKINT6, 
	NPC_WORK_ROUTEMAX = CHAR_NPCWORKINT7,
	NPC_WORK_WAITTIME = CHAR_NPCWORKINT8,
	NPC_WORK_CURRENTTIME = CHAR_NPCWORKINT9,
	NPC_WORK_SEFLG = CHAR_NPCWORKINT10,
};

/* 拒  メッセージのenum */
enum {
	NPC_BUS_MSG_GETTINGON,
	NPC_BUS_MSG_NOTPARTY,
	NPC_BUS_MSG_OVERPARTY,
	NPC_BUS_MSG_DENIEDITEM,
	NPC_BUS_MSG_ALLOWITEM,
	NPC_BUS_MSG_LEVEL,
	NPC_BUS_MSG_GOLD,
	NPC_BUS_MSG_EVENT,
	NPC_BUS_MSG_START,
	NPC_BUS_MSG_END,
};
typedef struct {
	char	option[32];
	char	defaultmsg[128];
}NPC_BUS_MSG;
NPC_BUS_MSG		busmsg[] = {
	{ "msg_gettingon",	"PAON�I�]�A�L�k�鵑こ~�[�Jи�稔髻I�^"},
	{ "msg_notparty",	"PAPAON�I�I�L�k�H肯兇�[�J�髻I"},
	{ "msg_overparty",	"PAON�I�I�H柴�w此�C"},
	{ "msg_denieditem",		"PAPAON�I�Iи�iぃ�n�o�唸D�磧I"},
	{ "msg_allowitem",		"�z梶~(�Q�n┷�唸D�祕�!)"},
	{ "msglevel",		"PAPAON�I�I�A�叉ク伝戮０��髻I"},
	{ "msg_stone",		"PAPAON�I�I��椙ぃ━�髻I"},
	{ "msg_event",		"PAON�I�I�A�L�k�[�J�髻I"},
	{ "msg_start",		"�z梶~(�X�o�i��)"},
	{ "msg_end",		"�z梶~(�戝�)"}
	
};

static int NPC_BusSetPoint( int meindex, char *argstr);
static void NPC_BusSetDestPoint( int meindex, char *argstr);
static BOOL NPC_BusCheckDeniedItem( int meindex, int charaindex, char *argstr);
static BOOL NPC_BusCheckLevel( int meindex, int charaindex, char *argstr);
static int NPC_BusCheckStone( int meindex, int charaindex, char *argstr);
static void NPC_BusSendMsg( int meindex, int talkerindex, int tablenum);
static int NPC_BusGetRoutePointNum( int meindex, char *argstr );
static void NPC_Bus_walk( int meindex);

#define		NPC_BUS_LOOPTIME		200

/* 待ち時間デフォルト */
#define		NPC_BUS_WAITTIME_DEFAULT	180

#define		NPC_BUS_WAITINGMODE_WAITTIME	5000

/*********************************
* 初期処  
*********************************/
BOOL NPC_BusInit( int meindex )
{
	char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
	char *argstr;
	int i;
	char	buf[256];
	int	routenum;
	int	waittime;
	int seflg;
	
	argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));

	/* なければいけない引数のチェック */
	routenum = NPC_Util_GetNumFromStrWithDelim( argstr, "routenum");//�羮p侯�u柴�q

	if( routenum == -1 ) {
		print( "npcbus:nothing routenum \n");
		return FALSE;
	}
	CHAR_setWorkInt( meindex, NPC_WORK_ROUTEMAX, routenum);
	
	for( i = 1; i <= routenum; i ++ ) {
		char routetostring[64];
		snprintf( routetostring, sizeof( routetostring), "routeto%d", i);
		if( NPC_Util_GetStrFromStrWithDelim( argstr, routetostring,buf, sizeof(buf))
			== NULL ) 
		{
			print( "npcbus:nothing route to \n");
			return FALSE;
		}

	}
	waittime = NPC_Util_GetNumFromStrWithDelim( argstr, "waittime");

	if( waittime == -1 ) waittime = NPC_BUS_WAITTIME_DEFAULT;
	CHAR_setWorkInt( meindex, NPC_WORK_WAITTIME, waittime);

	seflg = NPC_Util_GetNumFromStrWithDelim( argstr, "seflg");

	if( seflg == -1 ) seflg = TRUE;
	CHAR_setWorkInt( meindex, NPC_WORK_SEFLG, seflg);
    
    CHAR_setInt( meindex , CHAR_WHICHTYPE , CHAR_TYPEBUS );
	
	CHAR_setWorkInt( meindex, NPC_WORK_MODE, 0);
	CHAR_setWorkInt( meindex, NPC_WORK_ROUTEPOINT, 2);
	CHAR_setWorkInt( meindex, NPC_WORK_ROUNDTRIP, 0);
	CHAR_setWorkInt( meindex, NPC_WORK_CURRENTROUTE, 0);
			
	CHAR_setInt( meindex, CHAR_LOOPINTERVAL, 
						NPC_BUS_WAITINGMODE_WAITTIME);
    
    /* 現在の時間をセット */
    CHAR_setWorkInt( meindex, NPC_WORK_CURRENTTIME, NowTime.tv_sec);

    for( i = 0; i < CHAR_PARTYMAX; i ++) {
    	CHAR_setWorkInt( meindex, CHAR_WORKPARTYINDEX1 + i, -1);
    }
	
	/* ルート決定する */
{
	int rev;
	int r = CHAR_getWorkInt( meindex, NPC_WORK_ROUTEMAX);
	CHAR_setWorkInt( meindex, NPC_WORK_CURRENTROUTE, RAND( 1, r));
	//print( "route:%d\n",CHAR_getWorkInt( meindex, NPC_WORK_CURRENTROUTE));

	/*   ろスタート */
	rev = NPC_Util_GetNumFromStrWithDelim( argstr, "reverse");

	if( rev == 1 ) {
		int num = NPC_BusGetRoutePointNum( meindex, argstr);

		if( num <= 0 ) {
			print( "npcbus:Very strange�I\n");
			return FALSE;
		}
		CHAR_setWorkInt( meindex, NPC_WORK_ROUTEPOINT, num-1);
		CHAR_setWorkInt( meindex, NPC_WORK_ROUNDTRIP, 1);
	}
	/* ルートをセットする */
	NPC_BusSetPoint( meindex, argstr);
	/* 行き先を  示する */
	NPC_BusSetDestPoint( meindex, argstr);
}

    return TRUE;
}


/*********************************
*   しかけられた時の処  
*********************************/
void NPC_BusTalked( int meindex , int talkerindex , char *szMes ,
                     int color )
{
    int i;
    int	partyflg = FALSE;
	
    /* プレイヤーに対してだけ  応する */
    if( CHAR_getInt( talkerindex , CHAR_WHICHTYPE ) != CHAR_TYPEPLAYER ) {
    	return;
    }
	/* 自分のパー  ィ  乗客  かどうか調べる */
	for( i = 0; i < CHAR_PARTYMAX; i ++ ) {
		int index = CHAR_getWorkInt( meindex, CHAR_WORKPARTYINDEX1+i);
		if( CHAR_CHECKINDEX(index)){
			if( index == talkerindex) {
				partyflg = TRUE;
			}
		}
	}
	if( !partyflg ) {
		//NPC_BusCheckJoinParty( meindex, talkerindex, TRUE);
	}
	else {
		if( CHAR_getWorkInt( meindex, NPC_WORK_MODE) == 0 ) {
			int i;
	//		#define NPC_BUS_DEBUGROUTINTG	"routingtable:"
			if( strstr( szMes, "�X�o" )  ||
				strstr( szMes, "�X�o" )  ||
				strstr( szMes, "Go" )  ||
				strstr( szMes, "go" ))
			{
				CHAR_setWorkInt( meindex, NPC_WORK_MODE,1);
				
				/* ループ関数の呼出しを�い�速度にする */
	 			CHAR_setInt( meindex, CHAR_LOOPINTERVAL, NPC_BUS_LOOPTIME);
				/* SE   らす  マンモスの叫び   */
				if( CHAR_getWorkInt( meindex, NPC_WORK_SEFLG )) {
					CHAR_sendSEoArroundCharacter( 
									CHAR_getInt( meindex, CHAR_FLOOR),
									CHAR_getInt( meindex, CHAR_X),
									CHAR_getInt( meindex, CHAR_Y),
									60,
									TRUE);
				}
				/* 出  する時のメッセージ*/
				for( i = 1; i < CHAR_PARTYMAX; i ++ ) {
					int partyindex = CHAR_getWorkInt( meindex, CHAR_WORKPARTYINDEX1+i);
					if( CHAR_CHECKINDEX( partyindex)) {
						NPC_BusSendMsg( meindex, partyindex, NPC_BUS_MSG_START);
					}
				}
			}
		}
#if 0
		else if( strstr( szMes, "葦ゎ" )  ||
			strstr( szMes, "葦ゎ" )  ||
			strstr( szMes, "stop" )  ||
			strstr( szMes, "Stop" ))
		{
			CHAR_setWorkInt( meindex, NPC_WORK_MODE,2);

			/* ループ関数のインターバルを多くする  */
			CHAR_setInt( meindex, CHAR_LOOPINTERVAL, 
						NPC_BUS_WAITINGMODE_WAITTIME);
		    /* 現在の時間をセット */
		    CHAR_setWorkInt( meindex, NPC_WORK_CURRENTTIME, NowTime.tv_sec);
		}
		else if( strstr( szMes, NPC_BUS_DEBUGROUTINTG )) {
			/* デバッグ用 */
			char *p = strstr( szMes,NPC_BUS_DEBUGROUTINTG);
			char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
			char *argstr;

			argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));
			if( p) {
				int a = atoi( p+strlen(NPC_BUS_DEBUGROUTINTG));
				if( a <0 ) a = 1;
				CHAR_setWorkInt( meindex, NPC_WORK_CURRENTROUTE, a);
			}
			//print( "route:%d\n",CHAR_getWorkInt( meindex, NPC_WORK_CURRENTROUTE));
			/* ルートをセットする */
			NPC_BusSetPoint( meindex, argstr);
		}
#endif
	}
}
/**************************************
 * ループ関数
 **************************************/
void NPC_BusLoop( int meindex)
{
	int	i;
	switch( CHAR_getWorkInt( meindex, NPC_WORK_MODE )) {
	  case 0:
	    /* 待ちモードの時�f時間をチェックする */
		/* 時間が経ったので�f出  する */
		if( CHAR_getWorkInt( meindex, NPC_WORK_CURRENTTIME) 
			+ CHAR_getWorkInt( meindex, NPC_WORK_WAITTIME) 
			< NowTime.tv_sec)
		{
			/* SE   らす  マンモスの叫び   */
			if( CHAR_getWorkInt( meindex, NPC_WORK_SEFLG )) {
				CHAR_sendSEoArroundCharacter( 
								CHAR_getInt( meindex, CHAR_FLOOR),
								CHAR_getInt( meindex, CHAR_X),
								CHAR_getInt( meindex, CHAR_Y),
								60,
								TRUE);
			}
			/* 出  する時のメッセージ*/
			for( i = 1; i < CHAR_PARTYMAX; i ++ ) {
				int partyindex = CHAR_getWorkInt( meindex, CHAR_WORKPARTYINDEX1+i);
				if( CHAR_CHECKINDEX( partyindex)) {
					NPC_BusSendMsg( meindex, partyindex, NPC_BUS_MSG_START);
				}
			}
			
			CHAR_setWorkInt( meindex, NPC_WORK_MODE,1);
			/* ループ関数の呼出しを�い�速度にする */
			CHAR_setInt( meindex, CHAR_LOOPINTERVAL, NPC_BUS_LOOPTIME);
		}
		return;
	  case 1:
	  	/* �い� */
	  	NPC_Bus_walk( meindex);
	  case 2:
		/* 止まっているモード */
		/* 時間が経ったので�f出  する */
		if( CHAR_getWorkInt( meindex, NPC_WORK_CURRENTTIME) 
			+ (CHAR_getWorkInt( meindex, NPC_WORK_WAITTIME) /3)
			< NowTime.tv_sec)
		{
			CHAR_setWorkInt( meindex, NPC_WORK_MODE,1);
			/* ループ関数の呼出しを�い�速度にする */
			CHAR_setInt( meindex, CHAR_LOOPINTERVAL, NPC_BUS_LOOPTIME);
		
		}
		return;
	  case 3:
		/* 到着しても�fクライアントの  写待ちの為に�f
		 * 少しここでウェイトをいれてやる
		 */
		if( CHAR_getWorkInt( meindex, NPC_WORK_CURRENTTIME) + 3
			< NowTime.tv_sec)
		{
			char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
			char *argstr;

			argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));
			/* ループ関数の呼出しを遅くする */
			CHAR_setInt( meindex, CHAR_LOOPINTERVAL, 
						NPC_BUS_WAITINGMODE_WAITTIME);
			
			/* ルート決定する */
			{
				int r = CHAR_getWorkInt( meindex, NPC_WORK_ROUTEMAX);
				CHAR_setWorkInt( meindex, NPC_WORK_CURRENTROUTE, RAND( 1, r));
				//print( "route:%d\n",CHAR_getWorkInt( meindex, NPC_WORK_CURRENTROUTE));
			}
			/* 行き帰りフラグ  更 */
			CHAR_setWorkInt( meindex, NPC_WORK_ROUNDTRIP, 
							CHAR_getWorkInt( meindex, NPC_WORK_ROUNDTRIP)^1);

			/* 鵜ポイントの調節 */
			/* 帰りは  殊処   */
			if( CHAR_getWorkInt( meindex, NPC_WORK_ROUNDTRIP) == 1)  {
				/* そのルートの  高ポイント数を  る */
				int num = NPC_BusGetRoutePointNum( meindex, argstr);
				CHAR_setWorkInt( meindex, NPC_WORK_ROUTEPOINT, num-1);
			}
			else {
				CHAR_setWorkInt( meindex, NPC_WORK_ROUTEPOINT, 
							CHAR_getWorkInt( meindex, NPC_WORK_ROUTEPOINT) +1);
			}
			/* 鵜のポイントの座  をセットする */
			NPC_BusSetPoint( meindex, argstr);
			/* 行き先を  示する */
			NPC_BusSetDestPoint( meindex, argstr);
			/* パー  ィ  ける処  をする */
			CHAR_DischargeParty( meindex, 0);
		    /* 現在の時間をセット */
		    CHAR_setWorkInt( meindex, NPC_WORK_CURRENTTIME, NowTime.tv_sec);
			/* モードクリア */
			CHAR_setWorkInt( meindex, NPC_WORK_MODE, 0);
		}
		return;
	  default:
	    break;
	}
}
/**************************************
 * �い��e
 **************************************/
static void NPC_Bus_walk( int meindex)
{	
	POINT	start, end;
	int dir;
	int ret;
	int i;

	/* �い�関係 */
	/* 到着した時の処   */
	start.x = CHAR_getInt( meindex, CHAR_X);
	start.y = CHAR_getInt( meindex, CHAR_Y);
	end.x = CHAR_getWorkInt( meindex, NPC_WORK_ROUTETOX);
	end.y = CHAR_getWorkInt( meindex, NPC_WORK_ROUTETOY);
	
	/* 到着したので鵜のポイントに */
	if( start.x == end.x && start.y == end.y ) {
		int add = 1;
		char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
		char *argstr;

		argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));

		if( CHAR_getWorkInt( meindex, NPC_WORK_ROUNDTRIP ) == 1 ) {
			add *= -1;
		}
		CHAR_setWorkInt( meindex, NPC_WORK_ROUTEPOINT, 
						CHAR_getWorkInt( meindex, NPC_WORK_ROUTEPOINT) +add);
		if( NPC_BusSetPoint( meindex, argstr) == FALSE ) {
			/*     に到着*/
			/* 待ちモードにする */
			CHAR_setWorkInt( meindex, NPC_WORK_MODE,3);
			
			/* SE   らす  マンモスの叫び   */
			if( CHAR_getWorkInt( meindex, NPC_WORK_SEFLG )) {
				CHAR_sendSEoArroundCharacter( 
								CHAR_getInt( meindex, CHAR_FLOOR),
								CHAR_getInt( meindex, CHAR_X),
								CHAR_getInt( meindex, CHAR_Y),
								60,
								TRUE);
			}
			/* 着いた時のメッセージ*/
			for( i = 1; i < CHAR_PARTYMAX; i ++ ) {
				int partyindex = CHAR_getWorkInt( meindex, CHAR_WORKPARTYINDEX1+i);
				if( CHAR_CHECKINDEX( partyindex)) {
					NPC_BusSendMsg( meindex, partyindex, NPC_BUS_MSG_END);
				}
			}
		    /* 現在の時間をセット */
		    CHAR_setWorkInt( meindex, NPC_WORK_CURRENTTIME, NowTime.tv_sec);
			return;
		}
		else {
			return;
		}
	}
	/*-------------------------------------------------------*/
	/* �い�せる処   */
	
	/*   向を求める */
	dir = NPC_Util_getDirFromTwoPoint( &start,&end );

	/* 今いる場所の待    パー  ィ�い�で使う   */
	end.x = CHAR_getInt( meindex, CHAR_X);
	end.y = CHAR_getInt( meindex, CHAR_Y);
	
	/* ひっかかった時の為の処   */
	for( i = 0; i < 100; i ++ ) {	
		if( dir < 0 ) {
			dir = RAND( 0,7);
		}	
		dir = NPC_Util_SuberiWalk( meindex, dir);
		if( dir >= 0 && dir <= 7) break;
	}
	
	if( dir >= 0 && dir <= 7 ) {
		/* �い� */
		ret = CHAR_walk( meindex, dir, 0);

		if( ret == CHAR_WALKSUCCESSED ) {
			/* 自分が親なら仲間を�い�せる */
			int	i;
			for( i = 1; i < CHAR_PARTYMAX; i ++ ) {
				int toindex = CHAR_getWorkInt( meindex, i + CHAR_WORKPARTYINDEX1);
				if( CHAR_CHECKINDEX(toindex) ) {
					int		parent_dir;
					/* 子の位  と�f親の�い�前の位  から  向を求める */
					/* �い� */
					start.x = CHAR_getInt( toindex, CHAR_X);
					start.y = CHAR_getInt( toindex, CHAR_Y);
					parent_dir = NPC_Util_getDirFromTwoPoint( &start,&end );
					/* グラディウスオプ竺ョン�い�を  現する為に�f
					 * 鵜の子は前の子の  を追うようにする
					 */
					end = start;
					if( parent_dir != -1 ) {
						CHAR_walk( toindex, parent_dir, 0);
					}
				}
			}
	    }
	}
}
/**************************************
 * 鵜の場所をセットする
 **************************************/
static int NPC_BusSetPoint( int meindex, char *argstr)
{
	char	buf[4096];
	char	buf2[256];
	char	buf3[256];
	int ret;
	char routetostring[64];
	
	snprintf( routetostring, sizeof( routetostring), "routeto%d", 
				CHAR_getWorkInt( meindex, NPC_WORK_CURRENTROUTE));
	
	if( NPC_Util_GetStrFromStrWithDelim( argstr, routetostring,buf, sizeof(buf))
		== NULL ) 
	{
		print( "npcbus:nothing route \n");
		return FALSE;
	}
	ret = getStringFromIndexWithDelim( buf, ";", 
								CHAR_getWorkInt( meindex, NPC_WORK_ROUTEPOINT),
								buf2, sizeof(buf2));
		
	if( ret == FALSE ) return FALSE;

	ret = getStringFromIndexWithDelim( buf2, ",", 1,
								buf3, sizeof(buf3));
	if( ret == FALSE) return FALSE;
	CHAR_setWorkInt( meindex, NPC_WORK_ROUTETOX, atoi( buf3));
	
	ret = getStringFromIndexWithDelim( buf2, ",", 2,
								buf3, sizeof(buf3));
	if( ret == FALSE) return FALSE;
	CHAR_setWorkInt( meindex, NPC_WORK_ROUTETOY, atoi( buf3));
	return TRUE;
}
/**************************************
 * route  号から�f  前があったらそれを
 * 称号のとこにセットする�e
 **************************************/
static void NPC_BusSetDestPoint( int meindex, char *argstr)
{
	char 	buf[256];
	char	routename[256];

	snprintf( routename, sizeof( routename), "routename%d", 
				CHAR_getWorkInt( meindex, NPC_WORK_CURRENTROUTE));

	if( NPC_Util_GetStrFromStrWithDelim( argstr, routename, buf, sizeof( buf))
		!= NULL ) 
	{
		CHAR_setChar( meindex, CHAR_OWNTITLE, buf);
		CHAR_sendCToArroundCharacter( CHAR_getWorkInt( meindex, CHAR_WORKOBJINDEX));
	}
}
/**************************************
 * 指定されたアイ  ムを  っているかチェックする
 *   っていたらだめ
 **************************************/
static BOOL NPC_BusCheckDeniedItem( int meindex, int charaindex, char *argstr)
{
	char	buf[1024];
	BOOL	found = TRUE;

	if( NPC_Util_GetStrFromStrWithDelim( argstr, "denieditem", buf, sizeof( buf))
		!= NULL ) 
	{
		int	i;
		int ret;
		for( i = 1; ; i ++) {
			int itemid;
			char buf2[64];
			int j;
			ret = getStringFromIndexWithDelim( buf, ",", i, buf2, sizeof(buf2));
			if( ret == FALSE ) break;
			itemid = atoi( buf2);
			for( j = 0; j < CHAR_MAXITEMHAVE; j ++) {
				int itemindex = CHAR_getItemIndex( charaindex, j);
				if( ITEM_CHECKINDEX( itemindex)) {
					if( ITEM_getInt( itemindex, ITEM_ID) == itemid) {
						found = FALSE;
						break;
					}
				}
			}
		}
	}
	return found;
}
/**************************************
 * 指定されたアイ  ムを  っているかチェックする
 *   っていないとだめ
 **************************************/
BOOL NPC_BusCheckAllowItem( int meindex, int charaindex, BOOL pickupmode)
{
	char	buf[1024];
	BOOL	found = TRUE;
	BOOL	pickup = FALSE;
	char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
	char *argstr;
	
	argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));
	
	if( NPC_Util_GetStrFromStrWithDelim( argstr, "pickupitem", buf, sizeof( buf))!= NULL ){
		pickup = TRUE;
	}
	if( NPC_Util_GetStrFromStrWithDelim( argstr, "allowitem", buf, sizeof( buf))!= NULL ){
		int	i;
		int ret;
		for( i = 1; ; i ++) {
			int itemid;
			char buf2[64];
			int j;
			BOOL	getflg;
			ret = getStringFromIndexWithDelim( buf, ",", i, buf2, sizeof(buf2));
			if( ret == FALSE ) break;
			itemid = atoi( buf2);
			getflg = FALSE;
			for( j = 0; j < CHAR_MAXITEMHAVE; j ++) {
				int itemindex = CHAR_getItemIndex( charaindex, j);
				if( ITEM_CHECKINDEX( itemindex)) {
					if( ITEM_getInt( itemindex, ITEM_ID) == itemid) {
						if( pickupmode && pickup && !getflg) {
							CHAR_DelItem( charaindex, j);
							getflg = TRUE;
						}
						break;
					}
				}
			}
			if( j == CHAR_MAXITEMHAVE) {
				found = FALSE;
				break;
			}
		}
	}
	return found;
}

/**************************************
 * 指定されたレベル以上かチェックする
 **************************************/
static BOOL NPC_BusCheckLevel( int meindex, int charaindex, char *argstr)
{
	int		level;
	
	/* なければいけない引数のチェック */
	level = NPC_Util_GetNumFromStrWithDelim( argstr, "needlevel");
	if( level == -1 ) {
		return TRUE;
	}
	if( CHAR_getInt( charaindex, CHAR_LV) >= level ) return TRUE;
	
	return FALSE;
}
/**************************************
 * 御金をチェックする
 * -1 駄   0以上�v    �fかつ  要Stone
 **************************************/
static int NPC_BusCheckStone( int meindex, int charaindex, char *argstr)
{
	int		gold;
	
	/* なければいけない引数のチェック */
	gold = NPC_Util_GetNumFromStrWithDelim( argstr, "needstone");
	if( gold == -1 ) {
		return 0;
	}
	if( CHAR_getInt( charaindex, CHAR_GOLD) >= gold ) return gold;
	
	return -1;
}
/**************************************
 * メッセージを送る
 * 引数のメッセージがなければデフォルトメッセージを送る
 **************************************/
static void NPC_BusSendMsg( int meindex, int talkerindex, int tablenum)
{
	char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
	char *argstr;
	char	buf[256];
	char	msg[256];
	if( tablenum < 0 || tablenum >= arraysizeof( busmsg)) return;
	
	argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));
	
	if( NPC_Util_GetStrFromStrWithDelim( argstr, busmsg[tablenum].option, buf, sizeof( buf))
		!= NULL ) 
	{
		strcpy( msg, buf);
	}
	else {
		snprintf( msg, sizeof(msg),busmsg[tablenum].defaultmsg);
	}
	CHAR_talkToCli( talkerindex, meindex, msg, CHAR_COLORYELLOW);
}
/**************************************
 * ルート  ーブルのポイントの数を取  する
 **************************************/
static int NPC_BusGetRoutePointNum( int meindex, char *argstr )
{
	int		i;
	char	buf[4096];
	char	buf2[256];
	int ret;
	char routetostring[64];
	
	snprintf( routetostring, sizeof( routetostring), "routeto%d", 
				CHAR_getWorkInt( meindex, NPC_WORK_CURRENTROUTE));
	
	if( NPC_Util_GetStrFromStrWithDelim( argstr, routetostring,buf, sizeof(buf))
		== NULL ) 
	{
		print( "npcbus:nothing route \n");
		return -1;
	}
	for( i = 1; ; i ++ ) {
		ret = getStringFromIndexWithDelim( buf, ";", i, buf2, sizeof(buf2));
		if( ret == FALSE) break;
	}
	return( i -1);
}
BOOL NPC_BusCheckJoinParty( int meindex, int charaindex, BOOL msgflg)
{
    //int		fd;
	char	argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
	char *argstr;
	int		ret;
	argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof( argstr1));
	/* １グリッド以  のみ */
	if( !NPC_Util_charIsInFrontOfChar( charaindex, meindex, 1 )) return FALSE; 
	/*     乗車は拒  する */
	if( CHAR_getWorkInt( meindex, NPC_WORK_MODE) != 0 ) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_GETTINGON);
		return FALSE;
	}
	/* ぱーてぃだったらだめ */
	if( CHAR_getWorkInt( charaindex, CHAR_WORKPARTYMODE ) != CHAR_PARTY_NONE) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_NOTPARTY);
		return FALSE;
	}
	/* パー  ィの人数をチェックする */
	if( CHAR_getEmptyPartyArray( meindex) == -1 ) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_OVERPARTY);
		return FALSE;
	}
	/* アイ  ムのチェックをする(禁止アイ  ム) */
	if( !NPC_BusCheckDeniedItem( meindex, charaindex, argstr)) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_DENIEDITEM);
		return FALSE;
	}
#ifdef _ITEM_CHECKWARES
	if( CHAR_CheckInItemForWares( charaindex, 0) == FALSE )	{
		CHAR_talkToCli( charaindex, -1, "�L�k亭�a�f���W┏�C", CHAR_COLORYELLOW);
		return FALSE;
	}
#endif
	/* アイ  ムのチェックをする(  要アイ  ム) */
	if( !NPC_BusCheckAllowItem( meindex, charaindex, FALSE)) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_ALLOWITEM);
		return FALSE;
	}
	/* レベルのチェックをする */
	if( !NPC_BusCheckLevel( meindex, charaindex, argstr)) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_LEVEL);
		return FALSE;
	}
	/* イベント  かチェックする */
//	if( CHAR_getInt( charaindex, CHAR_NOWEVENT) != 0 ||
//		CHAR_getInt( charaindex, CHAR_NOWEVENT2) != 0 ||
//		CHAR_getInt( charaindex, CHAR_NOWEVENT3) != 0 )
//	{
//		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_EVENT);
//		return FALSE;
//	}
	/* 御金のチェックをする  恭金を取るので�f  終チェックにすること！   */
	ret = NPC_BusCheckStone( meindex, charaindex, argstr);
	if( ret == -1 ) {
		if( msgflg) NPC_BusSendMsg( meindex, charaindex, NPC_BUS_MSG_GOLD);
		return FALSE;
	}
	if( ret != 0 ) {
		char msgbuf[128];
		/* 御金をとる */
		CHAR_setInt( charaindex, CHAR_GOLD, 
					CHAR_getInt( charaindex, CHAR_GOLD) - ret);
		/* 送信 */
		CHAR_send_P_StatusString( charaindex, CHAR_P_STRING_GOLD);
		snprintf( msgbuf, sizeof( msgbuf), "や�I�F%d Stone�I", ret);
		CHAR_talkToCli( charaindex, -1, msgbuf, CHAR_COLORYELLOW);
	}
	/* パー  ィに  る */
	//CHAR_JoinParty_Main( charaindex, meindex);
	
	//fd = getfdFromCharaIndex( charaindex );
	
	//lssproto_PR_send( fd, 1, 1);
	return TRUE;
}
