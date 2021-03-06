#include "version.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "util.h"
#include "log.h"
#include "handletime.h"
#include "net.h"
#include "char_base.h"

/*
 *
 * ひグファイルの種  はappend するものとしないものがある�e
 * appendするものは  初にファイルをひらいて恭く�e
 * そうでないものは書きこみのたびにfopen(..,"w")する
 * by ringo
 */

struct tagLogconf{
    char*   label;
    char*   entry;
    char    filename[256];
    FILE*   f;
    BOOL    append;             /* append するか�b書きこみのたびにSEEK_SETするか */
}LogConf[LOG_TYPE_NUM]={
    { "TALK: ", "talklog" ,"", NULL , FALSE},
    { "PROC: ", "proc" , "" , NULL , FALSE},
    { "ITEM: ", "itemlog" ,"", NULL , FALSE},
    { "STONE: ", "stonelog" ,"", NULL , FALSE},
    { "PET: ", "petlog" ,"", NULL , FALSE},
    { "TENSEI: ", "tenseilog" ,"", NULL , FALSE},
    { "KILL: ", "killlog","",NULL,FALSE},
    // CoolFish: 2001/4/19
    { "TRADE: ", "tradelog", "", NULL, FALSE},
    // Arminius: 2001/6/14
    { "HACK: ", "hacklog", "", NULL, FALSE},
    // Nuke: 0626 Speed
    { "SPEED: ", "speedlog", "", NULL, FALSE},
    // CoolFish: FMPopular 2001/9/12
    { "FMPOP: ", "fmpoplog", "", NULL, FALSE},
    // Robin 10/02
    { "FAMILY: ", "familylog", "", NULL, FALSE},
    // Shan 11/02
    { "GM: ", "gmlog", "", NULL, FALSE},
     // Terry 2001/09/28
#ifdef _SERVICE
    { "SERVICE: ", "servicelog", "", NULL, FALSE},       
#endif
//巡兜除�L	�_�_�p殉 
#ifdef _GAMBLE_ROULETTE
	{ "", "gamblelog", "", NULL, FALSE}, 
#endif
#ifdef _TEST_PETCREATE
	{ "", "creatpetlog", "", NULL, FALSE},
	{ "", "creatpetavglog", "", NULL, FALSE}, 
#endif
	{ "LOGIN: ", "loginlog", "", NULL, FALSE},
	{ "", "pettranslog", "", NULL, FALSE},
//Syu �W�[怪狗菖較�tLog
	{ "FMPKRESULT: ", "fmpkresultlog" ,"", NULL , FALSE},

// Syu ADD �s�W�a謁�咾H使�网s��Log (ぃ�t�a謁使��)
	{ "BANKSTONELOG: ", "bankstonelog" ,"", NULL , FALSE},

	{ "ACMESSAGE: ", "acmessagelog" ,"", NULL , FALSE},
	{ "PKCONTEND:", "pkcontendlog", "", NULL, FALSE},
//�\�y
#ifdef _STREET_VENDOR
	{ "STREETVENDOR: ", "StreetVendorlog" ,"", NULL , FALSE},
#endif
//埼�F�l滑
#ifdef _ANGEL_SUMMON
	{ "ANGEL: ", "angellog" ,"", NULL , FALSE},
#endif
// 鐸教�T�ПMノLOG
#ifdef _LOG_OTHER
	{ "OTHER: ", "otherlog" ,"", NULL , FALSE},
#endif
// �s怪狗�W�hLOG
#ifdef _NEW_MANOR_LAW
	{ "FMPKGETMONEY: ","FMPKGetMoneylog","",NULL,FALSE},
	{ "FMFAMESHOP: ","FMFameShoplog","",NULL,FALSE},
#endif
#ifdef _JZ_VIPLOG
	{ "VIPACTION: ","vipActionlog","",NULL,FALSE},
#endif
};

tagWarplog warplog[MAXMAPNUM];
tagWarpCount warpCount[MAXMAPLINK];

/*------------------------------------------------------------
 * ひグ設定ファイルを  んで file を開く
 * 引数
 *  filename        char*       ひグ設定ファイル  
 * 返り値
 *  FALSE   は重  な失  である�e
 ------------------------------------------------------------*/
static BOOL readLogConfFile( char* filename )
{
    FILE*   f;
    char    line[256];
    char    basedir[256];
    int     linenum=0;
    char    firstToken[256];
    char    secondToken[256];
    int     i;
    BOOL    ret;
    {
        char*   r;
        r = rindex( filename, '/' );
        if( r  == NULL )snprintf(basedir,sizeof(basedir),"." );
        else{
            memcpy( basedir,filename,r-filename );
            basedir[r-filename] = '\0';
        }
    }

    f = fopen( filename , "r");
    if( f == NULL ){
        print( "Can't open %s\n" , filename );
        return FALSE;
    }
    while( fgets( line, sizeof( line ) ,f ) ){

        linenum++;
        deleteWhiteSpace(line);          /* remove whitespace    */
        if( line[0] == '#' )continue;        /* comment */
        if( line[0] == '\n' )continue;       /* none    */
        chomp( line );                    /* remove tail newline  */
        ret = getStringFromIndexWithDelim( line , "=",  1, firstToken, sizeof(firstToken) );
        if( ret == FALSE ){
            print( "Find error at %s in line %d. Ignore\n",
                   filename , linenum);
            continue;
        }
        for( i=0 ; i<arraysizeof(LogConf) ; i++ ){
            if( strcmp( LogConf[i].entry, firstToken )== 0 ){
                ret = getStringFromIndexWithDelim( line, "=", 2,
                                                   firstToken,
                                                   sizeof(firstToken) );
                if( ret == FALSE ){
                    print( "Find error at %s in line %d. Ignore\n",
                           filename , linenum);
                    continue;
                }
                //�N�J ADD �W�[CF�]�m�Y�桧O雀匝�O�w�桧O雀
                ret = getStringFromIndexWithDelim( firstToken, ":", 1,
                                                   secondToken,
                                                   sizeof(secondToken) );
                if( ret == FALSE ){
	                snprintf( LogConf[i].filename,
	                          sizeof( LogConf[i].filename ),
	                          "%s/%s",basedir,firstToken);
                    continue;
                }else
                {
	                snprintf( LogConf[i].filename,
	                          sizeof( LogConf[i].filename ),
	                          "%s/%s",basedir,secondToken);
                }
                ret = getStringFromIndexWithDelim( firstToken, ":", 2,
                                                   secondToken,
                                                   sizeof(secondToken) );
                if( ret == FALSE ){
										LogConf[i].append = TRUE;
                    continue;
                }
                if(atoi(secondToken)){
										LogConf[i].append = TRUE;
								}else{
										LogConf[i].append = FALSE;
								}
            }
        }
    }
    fclose(f);
    return TRUE;
}

int openAllLogFile( void )
{
    int     i;
    int     opencount=0;
    for( i=0 ; i<arraysizeof(LogConf) ; i++ ){
        if( ! LogConf[i].append )continue;
        LogConf[i].f = fopen( LogConf[i].filename , "a+" );
        if( LogConf[i].f != NULL )opencount++;
    }
    return opencount;
}

void closeAllLogFile( void )
{
    int     i;
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));

	// WON FIX
    for( i=0 ; i<arraysizeof(LogConf) ; i++ ){
        if( LogConf[i].f && LogConf[i].append ){
			printl( i, "server down(%d:%d) " , tm1.tm_hour, tm1.tm_min);
			fclose( LogConf[i].f );
		}
	}

/*
    for( i=0 ; i<arraysizeof(LogConf) ; i++ )
        if( LogConf[i].f && LogConf[i].append )
            fclose( LogConf[i].f );
*/
}

void printl( LOG_TYPE logtype, char* format , ... )
{
    va_list arg;
    if( logtype < 0 || logtype >= LOG_TYPE_NUM )return;
    if( LogConf[logtype].append ){
        if( !LogConf[logtype].f )return;
        fputs( LogConf[logtype].label, LogConf[logtype].f);
        va_start(arg,format);
        vfprintf( LogConf[logtype].f,format,arg );
        va_end( arg );
        fputc( '\n', LogConf[logtype].f );
    } else {
        FILE *f = fopen( LogConf[logtype].filename ,"a+" );
        if( !f ) return;
        fputs(LogConf[logtype].label , f );
        va_start(arg,format);
        vfprintf( f , format,arg);
        va_end(arg);
        fputc( '\n' , f);
        fclose(f);
    }
}

BOOL initLog( char* filename )
{
    if( readLogConfFile( filename ) == FALSE )return FALSE;
    openAllLogFile();
    return TRUE;
}

//Syu �W�[怪狗菖較�tLog
void Logfmpk(
			 char *winner, int winnerindex, int num1,
			 char *loser, int loserindex, int num2,
			 char *date, char *buf1, char *buf2, int flg)
{
	switch( flg){
	case 1:
		{
			struct  tm tm1;
			char buf[256];
			memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
			sprintf( buf, " (%d:%d)", tm1.tm_hour, tm1.tm_min);
			printl( LOG_FMPKRESULT, "\nFMPK: [%s]�a�I:%s %s(%d) ��菖�n�D %s(%d) time:%s",
				buf1, buf2,
				winner, winnerindex, loser, loserindex, buf);
		}
		break;
	case 2:
		printl( LOG_FMPKRESULT, "\nFMPK: Winner %s(%d)=>%d Loser %s(%d)=>%d time:%s",
			winner, winnerindex, num1,
			loser, loserindex, num2 ,date);
		break;
	}
}

#ifdef _NEW_MANOR_LAW
void LogFMPKGetMomey(char *szFMName,char *szID,char *szCharName,int iMomentum,int iGetMoney,int iDest)
{
	struct  tm tm1;
	char szDest[3][6] = {"┃�W","使��","杉�~"};

	if(iDest < 0 || iDest > 1) iDest = 2;
	memcpy(&tm1,localtime((time_t*)&NowTime.tv_sec),sizeof(tm1));
 	printl(LOG_FMPK_GETMONEY,"FMName:%s\tID:%s\tName:%s\tMomentum:%d\tGetMoney:%d\tAddTo:%s\t(%d:%d)",
														szFMName,szID,szCharName,iMomentum,iGetMoney,szDest[iDest],tm1.tm_hour,tm1.tm_min);
}

void LogFMFameShop(char *szFMName,char *szID,char *szCharName,int iFame,int iCostFame)
{
	struct  tm tm1;

	memcpy(&tm1,localtime((time_t*)&NowTime.tv_sec),sizeof(tm1));
 	printl(LOG_FM_FAME_SHOP,"FMName:%s\tID:%s\tName:%s\tFame:%d\tCostFame:%d\t(%d:%d)",
														szFMName,szID,szCharName,iFame,iCostFame,tm1.tm_hour,tm1.tm_min);
}
#endif

void LogAcMess(	int fd, char *type, char *mess )
{
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	if( strstr( mess, "Broadcast") != NULL ) return;
 	printl( LOG_ACMESS, "%d %s [%s] (%d:%d)" , fd, type, mess, tm1.tm_hour, tm1.tm_min);
}

void LogItem(
	char *CharName, /* キャラクタ   */
        char *CharID, /* キャラクタID */	
	int ItemNo, 	/* アイ  ム  号 */
	char *Key, 		/* キーワード */
	int floor,		/* 座   */
	int x,
	int y,
	char *uniquecode, // shan 2001/12/14
	char *itemname, int itemID
){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));

 	printl( LOG_ITEM, "%s\t%s\t%d(%s)=%s,(%d,%d,%d)(%d:%d),%s" , CharName, CharID, 
		itemID,	itemname,	
		Key, floor, x, y, tm1.tm_hour, tm1.tm_min, uniquecode );


}
void LogPkContend( char *teamname1, char *teamname2,
	int floor,
	int x,
	int y,
	int flg
)
{
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));

	if( flg == 0 ) {
 		printl( LOG_PKCONTEND, "[%32s 較 %32s],(%5d,%4d,%4d)(%d:%d)" ,
			teamname1, teamname2, 
			floor, x, y, tm1.tm_hour, tm1.tm_min);
	}else{
 		printl( LOG_PKCONTEND, "Msg:[%s],(%5d,%4d,%4d)(%d:%d)" ,
			teamname1, floor, x, y, tm1.tm_hour, tm1.tm_min);
	}

}

void LogPetTrans(
	char *cdkey, char *uniwuecde, char *uniwuecde2,
	char *CharName, int floor, int x, int y,
	int petID1, char *PetName1, int petLV, int petrank, int vital1, int str1, int tgh1, int dex1, int total1,
	int petID2, char *PetName2, int vital2, int str2, int tgh2, int dex2, int total2,
	int work0, int work1, int work2, int work3, int ans, int trans
	){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	printl( PETTRANS, "\n*PETTRANS cdkey=%s unid=%s munid=%s %s (%d:%d)  %d=%s LV:%d rand:%d trans:%d :[ %d, %d, %d, %d]=%d  %d=%s :[ %d, %d, %d, %d]=%d  [ %d, %d, %d, %d]=%d\n",
		cdkey, uniwuecde, uniwuecde2,		
		CharName, tm1.tm_hour, tm1.tm_min,
				petID1, PetName1, petLV, petrank, trans, vital1, str1, tgh1, dex1, total1,
				petID2, PetName2, vital2, str2, tgh2, dex2, total2,
				work0, work1, work2, work3, ans	);
}                                                                                        
/*------------------------------------------------------------
 *
 * ペットひグを取る
 *
-------------------------------------------------------------*/
void LogPet(
	char *CharName, /* キャラクタ   */
	char *CharID,
	char *PetName,
	int  PetLv,
	char *Key, 		/* キーワード */
	int floor,		/* 座   */
	int x,
	int y,
	char *uniquecode  // shan 2001/12/14	
){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	// shan 2001/12/14
 	//printl( LOG_PET, "%s\t%s\t%s:%d=%s,(%d,%d,%d)(%d:%d)" , CharName, CharID,
 	//		PetName, PetLv,
 	//		Key,
 	//		floor, x, y, tm1.tm_hour, tm1.tm_min );
	printl( LOG_PET, "%s\t%s\t%s:%d=%s,(%d,%d,%d)(%d:%d),%s" , CharName, CharID,
 			PetName, PetLv,
 			Key,
 			floor, x, y, tm1.tm_hour, tm1.tm_min, uniquecode);
}

#ifdef _STREET_VENDOR
void LogStreetVendor(
 	char *SellName,
	char *SellID,
	char *BuyName,
	char *BuyID,
	char *ItemPetName,
	int PetLv, //�Y�O�D�礒広八� -1
	int iPrice,
	char *Key,
	int Sfloor,
	int Sx,
	int Sy,
	int Bfloor,
	int Bx,
	int By,
	char *uniquecode
){
	struct  tm tm1;
	memcpy(&tm1,localtime((time_t *)&NowTime.tv_sec),sizeof(tm1));
	printl(LOG_STREET_VENDOR,"Sell:%s\t%s\tBuy:%s\t%s\tName=%s:Lv=%d|Price:%d,%s,SXY(%d,%d,%d)BXY(%d,%d,%d)(%d:%d),%s",SellName,SellID,BuyName,BuyID,
 														ItemPetName,PetLv,iPrice,Key,Sfloor,Sx,Sy,Bfloor,Bx,By,tm1.tm_hour,tm1.tm_min,uniquecode);
}
#endif

void LogBankStone(
        char *CharName, /* キャラクタ   */
        char *CharId, /* ユーザーID */
		int	meindex,
        int Gold,               /* 金額 */
        char *Key,              /* キーワード */
        int floor,              /* 座   */
        int x,
        int y,
		int my_gold,
		int my_personagold

){
        struct  tm tm1;

        memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
        printl( LOG_STONE, "%s:%s\ts:%d=%s,(%d,%d,%d)(%d:%d) <<own=%d,bank=%d>>" ,
			CharId, CharName, Gold, Key,
        floor, x, y, tm1.tm_hour, tm1.tm_min , my_gold, my_personagold );
}

void LogPetPointChange( 
	char * CharName, char *CharID, char *PetName, int petindex, int errtype,
	int PetLv, char *Key,int floor, int x, int y)	{

	struct tm tm1;
	int vit,str,tgh,dex;
	int l_vit,l_str,l_tgh,l_dex;
	int pet_ID, levellvup;

	pet_ID = CHAR_getInt( petindex, CHAR_PETID );
	vit	= CHAR_getInt( petindex, CHAR_VITAL );
	str = CHAR_getInt( petindex, CHAR_STR );
	tgh = CHAR_getInt( petindex, CHAR_TOUGH );
	dex = CHAR_getInt( petindex, CHAR_DEX );
	levellvup = CHAR_getInt( petindex, CHAR_ALLOCPOINT);

	l_vit = (levellvup >> 24);
	l_str = (levellvup >> 16)&0xff;
	l_tgh = (levellvup >> 8 )&0xff;
	l_dex = (levellvup >> 0 )&0xff;

	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_PET, "%s\t%s\t%s:%d=%s,(%d,%d,%d)(%d:%d),err:%d %d<<%d,%d,%d,%d>>lvup<<%d,%d,%d,%d>>" , 
		CharName, CharID, PetName, PetLv, Key, floor, x, y, tm1.tm_hour, tm1.tm_min , errtype,
		pet_ID ,vit,str,tgh,dex,l_vit,l_str,l_tgh,l_dex);
}

/*------------------------------------------------------------
 *
 * 転生ひグを取る
 *
-------------------------------------------------------------*/
void LogTensei(
	char *CharName, /* キャラクタ   */
	char *CharID,
	char *Key, 		/* キーワード */
	int level,		//レベル
	int transNum,	//転生回数
	int quest,		//クエスト数
	int home,		//出身�M
	int item,		//  加アイ  ム  かり数
	int pet,		//  加ペット  かり数
	int vital,		//  化前Vital
	int b_vital,	//  化  vital
	int str,		//  化前str
	int b_str,		//  化  str
	int tgh,		//  化前      
	int b_tgh,		//  化        
	int dex,		//  化前      
	int b_dex		//  化        
){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_TENSEI, "%s\t%s\t%s=(%d,%d,%d,%d,%d,%d),(vi=%d->%d,str=%d->%d,tgh=%d->%d,dex=%d->%d),(%d,%d)"
 			,CharName, 
 			CharID,
 			Key,
 			level,
 			transNum,
 			quest,
 			home,
 			item,
 			pet,
 			vital,
 			b_vital,
 			str,
 			b_str,
 			tgh,
 			b_tgh,
 			dex,
 			b_dex,
 			tm1.tm_hour, tm1.tm_min
 			 );
}

// LOG_TALK
void LogTalk(
	char *CharName, /* キャラクタ   */
	char *CharID,
	int floor,		/* 座   */
	int x,
	int y,
	char *message
){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));

	printl( LOG_TALK, "%2d:%2d\t%s\t%s\t%d_%d_%d\tT=%s" ,
		tm1.tm_hour, tm1.tm_min,
		(CharID==NULL) ? "(null)" :CharID,
		(CharName==NULL) ? "(null)" :CharName,
		floor, x, y,
		message );


}
#ifdef _TEST_PETCREATE
void backupTempLogFile( char *buf, char *entryname, int Num)
{
  int     i;
  char szBuffer[256];
	//entry
    for( i=0 ; i<arraysizeof(LogConf) ; i++ ){
        if( ! LogConf[i].append )continue;
		if( strcmp( LogConf[i].entry , entryname )	)
			continue;
		print("\n\n find entryname !!");
		sprintf( szBuffer, "./log/%s.%d", buf, Num );
		print("\n backup %s \n", szBuffer );
		if( LogConf[i].f != NULL ){
			fclose( LogConf[i].f );
			rename( LogConf[i].filename, szBuffer );
	        LogConf[i].f = fopen( LogConf[i].filename , "a+" );
		}else{
			rename( LogConf[i].filename, szBuffer );
	        LogConf[i].f = fopen( LogConf[i].filename , "a+" );
		}
		break;
    }
}
#endif
/*------------------------------------------------------------
 * 設定にしたがってすべてのファイルをバックアップ
 * ファイルはクひー�Nされていなければクひー�Nする
 * 引数  struct tm
 *  なし
 * 返り値
 *  オープンしたファイルの数
 ------------------------------------------------------------*/
void backupAllLogFile( struct tm *ptm )
{
    int     i;
    char szBuffer[256];

    for( i=0 ; i<arraysizeof(LogConf) ; i++ ){
        /* append でないものはしない */
        if( ! LogConf[i].append )continue;

		/* バックアップファイル  作�� */
		sprintf( szBuffer, "%s.%4d%02d%02d", LogConf[i].filename,
			ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday );

		if( LogConf[i].f != NULL ){
			/* オープンされていたらクひー�N */
			fclose( LogConf[i].f );
			/* リネーム */
			rename( LogConf[i].filename, szBuffer );
			/* 再びオープン */
	        LogConf[i].f = fopen( LogConf[i].filename , "a+" );

		}else{
			/* リネーム */
			rename( LogConf[i].filename, szBuffer );
			/* 再びオープン */
	        LogConf[i].f = fopen( LogConf[i].filename , "a+" );

		}
    }
}
/*------------------------------------------------------------
*
* 恭金を拾う
*
-------------------------------------------------------------*/
// Syu ADD �s�W�a謁�咾H使�网s��Log (ぃ�t�a謁使��)
void LogFamilyBankStone(
        char *CharName,
        char *CharId, 
        int Gold,     
		int MyGold,
        char *Key,    
        int floor,    
        int x,
        int y,
		int banksum
){
        struct  tm tm1;
        memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
        printl( LOG_BANKSTONELOG, "%s:%s\t%d=%s [%d] CHAR_GOLD(%d),(%d,%d,%d)(%d:%d)" , CharId, CharName, Gold, Key,banksum,
			MyGold, floor, x, y, tm1.tm_hour, tm1.tm_min );
		print("\n%s:%s\t%d=%s [%d] CHAR_GOLD(%d),(%d,%d,%d)(%d:%d)\n" , CharId, CharName, Gold, Key,banksum,
			MyGold, floor, x, y, tm1.tm_hour, tm1.tm_min );
}

void LogStone(
				int TotalGold,
        char *CharName, /* キャラクタ   */
        char *CharId, /* ユーザーID */
        int Gold,               /* 金額 */
		int MyGold,
        char *Key,              /* キーワード */
        int floor,              /* 座   */
        int x,
        int y
){
        struct  tm tm1;
        memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
				if(TotalGold == -1){
					printl( LOG_STONE, "%s:%s\t%d=%s TOTAL_GOLD(%d),CHAR_GOLD(%d),(%d,%d,%d)(%d:%d)" , CharId, CharName, Gold, Key,TotalGold,
					MyGold, floor, x, y, tm1.tm_hour, tm1.tm_min );
				}
				else{
					printl( LOG_STONE, "%s:%s\t%d=%s CHAR_GOLD(%d),(%d,%d,%d)(%d:%d)" , CharId, CharName, Gold, Key,
					MyGold, floor, x, y, tm1.tm_hour, tm1.tm_min );
				}
}

//ttom 12/26/2000 print the kill log
void LogKill(
     char *CharName,
     char *CharId,
     char *CharPet_Item
){
     struct  tm tm1;
     memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
     printl( LOG_KILL, "Name=%s:ID=%s\t%s (%d:%d)" ,CharName,CharId, CharPet_Item,
     tm1.tm_hour, tm1.tm_min );
}
//ttom

// CoolFish: Trade 2001/4/19
void LogTrade(char *message)
{
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_TRADE, "%s (%d:%d)" , message, tm1.tm_hour, tm1.tm_min );
}

// CoolFish: Family Popular 2001/9/12
void LogFMPOP(char *message)
{
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	
 	printl( LOG_FMPOP, "%s (%d:%d)" , message, tm1.tm_hour, tm1.tm_min );
}

// Arminius 2001/6/14
char hackmsg[HACK_TYPE_NUM][4096]=
	{ "??? ぐ賜�討]�SΤ�oネ",
	  "�L�k���o�q�T�鶇w�X",
	  "Μ�豕L�k遂恥�些q�T�鶇w�X",
	  "惜�d�X杉�~",
      "�H����HP�悪t",  
	};

void logHack(int fd, int errcode)
{
	struct tm tm1;
	char cdkey[4096];
	char charname[4096];
	unsigned long ip;
	char ipstr[4096];
	
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	CONNECT_getCdkey( fd, cdkey, 4096);
	CONNECT_getCharname( fd, charname, 4096);
	ip=CONNECT_get_userip(fd);
	sprintf(ipstr,"%d.%d.%d.%d",
        	((unsigned char *)&ip)[0],
        	((unsigned char *)&ip)[1],
        	((unsigned char *)&ip)[2],
        	((unsigned char *)&ip)[3]);
        if ((errcode<0) || (errcode>=HACK_TYPE_NUM)) errcode=HACK_NOTHING;

        printl( LOG_HACK, "(%d:%d) %s ip=%s cdkey=%s charname=%s",
        	tm1.tm_hour, tm1.tm_min, hackmsg[errcode], ipstr, cdkey, charname);
}

// Nuke 0626
void logSpeed(int fd)
{
	struct tm tm1;
	char cdkey[4096];
	char charname[4096];
	unsigned long ip;
	char ipstr[4096];
	
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	CONNECT_getCdkey( fd, cdkey, 4096);
	CONNECT_getCharname( fd, charname, 4096);
	ip=CONNECT_get_userip(fd);
	sprintf(ipstr,"%d.%d.%d.%d",
        	((unsigned char *)&ip)[0],
        	((unsigned char *)&ip)[1],
        	((unsigned char *)&ip)[2],
        	((unsigned char *)&ip)[3]);
	printl( LOG_SPEED, "(%d:%d) ip=%s cdkey=%s charname=%s",
        	tm1.tm_hour, tm1.tm_min, ipstr, cdkey, charname);
}

// Shan 
void LogGM(
        char *CharName,    //┐�皃W細
        char *CharID,      //�窺aID
        char *Message,     //���Oず�e
        int  floor,
        int  x,
        int  y
)
{
  struct  tm tm1;
                                                          
  memcpy(&tm1,localtime((time_t *)&NowTime.tv_sec),sizeof(tm1));
  printl(LOG_GM,"%s\t%s\t%s\t(%d,%d,%d)\t(%d:%d)",
         CharName,CharID,Message,floor,x,y,tm1.tm_hour,tm1.tm_min);    
}

// Robin 10/02
void LogFamily(
	char *FMName,
	int fmindex,
	char *charName,
	char *charID,
	char *keyWord,
	char *data
){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
	// CoolFish: 2001/10/11 log time
 	printl( LOG_FAMILY, "%s\t%d\t%s\t%s\t= %s, %s (%d:%d)",
 		FMName, fmindex, charName, charID,
 		keyWord, data, tm1.tm_hour, tm1.tm_min );
}

// Terry 2001/09/28
#ifdef _SERVICE
void LogService(
        char *CharName, //┐�皃W細
        char *CharID,   //�窺aID
        int  itemid,    //���~ID
        char *Key,      //察��
        int floor,
        int x,
        int y
)
{
  struct  tm tm1;
                                                          
  memcpy(&tm1,localtime((time_t *)&NowTime.tv_sec),sizeof(tm1));
  printl(LOG_SERVICE,"%s\t%s\t%d=%s,(%d,%d,%d)(%d:%d)",
         CharName,CharID,itemid,Key,floor,x,y,tm1.tm_hour,tm1.tm_min);
  print("%s\t%s\t%d=%s,(%d,%d,%d)(%d:%d)",
         CharName,CharID,itemid,Key,floor,x,y,tm1.tm_hour,tm1.tm_min);
}
#endif

#ifdef _TEST_PETCREATE
void LogCreatPet(
	char *PetName, int petid, int lv, int hp,
	int char_vital, int char_str, int char_tgh, int char_dex,
	int vital, int str, int tgh, int dex,
	int fixstr, int fixtgh, int fixdex,
	int lvup, int petrank,
	int flg
	)	{
  struct  tm tm1;
  memcpy(&tm1,localtime((time_t *)&NowTime.tv_sec),sizeof(tm1));
  if( flg == 0 )	{
	printl(LOG_CREATPET,"%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,power,%d,%d,%d",
		PetName, petid, lv, hp, char_vital/100, char_str/100, char_tgh/100, char_dex/100,
		vital, str, tgh, dex, lvup, petrank,fixstr,fixtgh,fixdex);
  }else	{
	printl(LOG_AVGCREATPET,"%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,power,%d,%d,%d",
		PetName, petid, lv, hp, char_vital/100, char_str/100, char_tgh/100, char_dex/100,
		vital, str, tgh, dex, lvup, petrank,fixstr,fixtgh,fixdex);
  }
}
#endif

#ifdef _GAMBLE_ROULETTE
void LogGamble(
        char *CharName, //┐�皃W細
        char *CharID,   //�窺aID
        char *Key,      //察��
        int floor,
        int x,
        int y,
		int player_stone,	//�匸乢蟹�椙
		int Gamble_stone,	//�U�`セ��
		int get_stone,		//栓�o
		int Gamble_num,
		int flg	//flg = 1 �窺a 2 怪�a
)
{
  struct  tm tm1;
  memcpy(&tm1,localtime((time_t *)&NowTime.tv_sec),sizeof(tm1));

  if( flg == 1 )	{
	printl(LOG_GAMBLE,"%s\t%s\t TYPE:%s  <<P_STONE:%9d,G_STONE:%9d,GET:%9d >>\t(%d,%d,%d)-(%d:%d) GAMBLENUM=%d",
         CharName,CharID,Key, player_stone, Gamble_stone, get_stone, floor,x,y,tm1.tm_hour,tm1.tm_min, Gamble_num);
  }else	if( flg == 2 )	{
	printl(LOG_GAMBLE,"%s\tROULETTE MASTER\t TYPE:%s  <<MASTER_STONE:%24d >>\t(%d,%d,%d)-(%d:%d)",
         CharName,Key, player_stone, floor,x,y,tm1.tm_hour,tm1.tm_min);
  }
}

#endif

void LogLogin(
        char *CharID,   //�窺aID
        char *CharName, //┐�皃W細
		int  saveIndex,
		char *ipadress
)
{
	struct  tm tm1;
                                                          
	memcpy(&tm1,localtime((time_t *)&NowTime.tv_sec),sizeof(tm1));

	printl(LOG_LOGIN,"%s\t%s\ti=%d\t%s\t(%d:%d)",
			CharID,CharName,saveIndex,ipadress,tm1.tm_hour,tm1.tm_min);

}


void warplog_to_file()
{
	int i =0;
	char outbuf[128];
	FILE *f;
	f = fopen("log/warp1.log" ,"w" );
	if( !f ) return;

	for( i=0; i<MAXMAPNUM; i++) {
		if( warplog[i].floor <= 0 )	continue;
		sprintf( outbuf, "%6d,%10d,%10d\n", warplog[i].floor, warplog[i].incount, warplog[i].outcount  );
		fputs( outbuf, f);
	}
	fclose(f);
	
	f = fopen("log/warp2.log" ,"w" );
	if( !f ) return;

	for( i=0; i<MAXMAPLINK; i++) {
		if( warpCount[i].floor1 <= 0 )	continue;
		sprintf( outbuf, "%6d,%6d,%10d\n", warpCount[i].floor1, warpCount[i].floor2, warpCount[i].count  );
		fputs( outbuf, f);
	}
	fclose(f);	

}

void warplog_from_file()
{
	int i =0;
	char outbuf[128];
	FILE *f;
	
	print("warplog_from_file ");

	f = fopen("log/warp1.log" ,"r" );
	if( !f ) return;

	while( fgets( outbuf, sizeof(outbuf), f) && i < MAXMAPNUM ) {
		
		if( !sscanf( outbuf, "%d,%d,%d",
			&warplog[i].floor, &warplog[i].incount, &warplog[i].outcount ) ) {
			
			continue;
			
		}
		//print(" %d", warplog[i].floor);
		i++;
	}
	print(" read_count:%d\n", i);
	
	fclose( f );


	f = fopen("log/warp2.log" ,"r" );
	if( !f ) return;

	i = 0;
	while( fgets( outbuf, sizeof(outbuf), f) && i < MAXMAPLINK ) {
		
		if( !sscanf( outbuf, "%d,%d,%d",
			&warpCount[i].floor1, &warpCount[i].floor2, &warpCount[i].count ) ) {
			
			continue;
			
		}
		i++;
	}
	print(" read_count2:%d\n", i);
	
	fclose( f );


}


void LogPetFeed( 
	char * CharName, char *CharID, char *PetName, int petindex,
	int PetLv, char *Key,int floor, int x, int y, char *ucode)	{

	struct tm tm1;

	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_PET, "%s\t%s\t%s:%d   �J=%s (%d,%d,%d)(%d:%d) %s " , 
		CharName, CharID, PetName, PetLv, Key, floor, x, y, tm1.tm_hour, tm1.tm_min, ucode);
}

#ifdef _ANGEL_SUMMON
void LogAngel( char *msg) {

	struct tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_ANGEL, "%s (%d:%d) ", msg, tm1.tm_hour, tm1.tm_min);
}
#endif

#ifdef _LOG_OTHER
void LogOther(
	char *CharName,
	char *CharID,
	char *message
){
	struct  tm tm1;
	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_OTHER, "%s\t%s\t(%d:%d)\t%s" , CharName, CharID, tm1.tm_hour, tm1.tm_min, message);
}
#endif

#ifdef _JZ_VIPLOG
void LogVipAction( 
	char *CharName,								//�H���W細
	char *Key,											//�b弦
	char *obj,											//刻抗�W�W細
	char *logmess,										//各漁
	int itemindex,									//刻抗ID
	int floor, 											//�H���勁b�a肱
	int x, int y, 									//�H���勁bГ斜
	char *ucode)										//�D�秉s�X
	{

	struct tm tm1;

	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_VIP_ACTION, "�b弦:%s-%s %s-%d(%d,%d,%d)(Time:%d-%d) code:%s 各漁:%s" , 
		Key, CharName, obj, itemindex, floor, x, y, tm1.tm_hour, tm1.tm_min, ucode,logmess);
}

void LogVipShop( 
	char *CharName,									//�H���W細
	char *Key,											//�b弦
	int memberq1,										//訴�R�e�嵯|���I
	int memberh1,										//訴�R�瓷嵯|���I
	int floor, 											//�H���勁b�a肱
	int x, int y, 									//�H���勁bГ斜
	char *logmess										//各漁
	)
	{

	struct tm tm1;

	memcpy( &tm1, localtime( (time_t *)&NowTime.tv_sec), sizeof( tm1));
 	printl( LOG_VIP_ACTION, "�b弦:%s-%s %d-%d(%d,%d,%d)(Time:%d-%d) 各漁:%s" , 
		Key, CharName, memberq1, memberh1, floor, x, y, tm1.tm_hour, tm1.tm_min,logmess);
}
#endif

