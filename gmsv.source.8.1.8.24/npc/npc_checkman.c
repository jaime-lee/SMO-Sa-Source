#include "version.h"
#include <string.h>
#include "char.h"
#include "object.h"
#include "char_base.h"
#include "npcutil.h"
#include "lssproto_serv.h"


//現在の使用できるフラグの数
#define MAXEVENTFLG 96

/*
 * イベントのフラグをチェックする      
 *
 */
 
static void NPC_CheckMan_selectWindow( int meindex, int toindex, int num);
int NPC_NowFlgCheck(int meindex,int talker,int now[MAXEVENTFLG]);
int NPC_EndFlgCheck(int meindex,int talker ,int nowflg[MAXEVENTFLG]);
BOOL NPC_FlgCheckMain( int meindex,int talker,int nowindex,int now[MAXEVENTFLG],char *work2);

/*********************************
* 初期処  
*********************************/
BOOL NPC_CheckManInit( int meindex )
{
	/*--キャラのタイプを設定--*/
    CHAR_setInt( meindex , CHAR_WHICHTYPE , CHAR_TYPECHECKMAN);

	return TRUE;
}


/*********************************
*   しかけられた時の処  
*********************************/
void NPC_CheckManTalked( int meindex , int talkerindex , char *szMes ,int color )
{

    /* プレイヤーに対してだけ  応する */
    if( CHAR_getInt( talkerindex , CHAR_WHICHTYPE ) != CHAR_TYPEPLAYER )
    {
    	return;
    }

	/*--  の前にいるかどうか！--*/
	if(NPC_Util_isFaceToFace( meindex ,talkerindex , 2) == FALSE) {
		/* １グリッド以  のみ */
		if(NPC_Util_isFaceToChara( talkerindex, meindex, 1) == FALSE) return;
	}

	//  初のウインドウに
	NPC_CheckMan_selectWindow( meindex, talkerindex, 0);

}


/*
 * 各処  に分ける
 */
static void NPC_CheckMan_selectWindow( int meindex, int talker, int num)
{

	char token[1024];
	char work[256];
	char work2[512];

	char escapedname[1024];
	int fd = getfdFromCharaIndex( talker);
	int buttontype = 0;
	int windowtype = 0;
	int windowno = 0;
	int now[MAXEVENTFLG];
	int nowindex;
	int i;	
	int page;
	
	work[0] = 0;
	work2[0] = 0;
	token[0] = 0;

	now[0] =0;
	
	/*--ウインドウタイプメッセージがおおいので先に設定--*/
  	windowtype = WINDOW_MESSAGETYPE_MESSAGE;

	switch( num) {
	
	  case 0:
		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANT,0);
		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC,0);
		
  		/*--選      --*/
		sprintf(token,"3\n �a�a�a�a「「�aチェックマン�a「「 "
				  "\n�a�a�a現在のフラグチェックをしまーす"
				  "\n"
				  "\n�a�a�a�a ≪�aNOWフラグチェック�a≫ "
				  "\n�a�a�a�a ≪�aENDフラグチェック�a≫ "
				  "\n\n�a�a ≪�aNOWフラグチェック  詳細  ≫ "
				  "\n�a�a ≪�aENDフラグチェック  詳細  ≫ "
		);

	  	buttontype = WINDOW_BUTTONTYPE_CANCEL;
	  	windowtype = WINDOW_MESSAGETYPE_SELECT;
	  	windowno = CHAR_WINDOWTYPE_CHECKMAN_START; 
	  	break;

	//NOWフラグの簡単  示
	  case 1:
	  	
	  	//NOWフラグのチェック
		nowindex = NPC_NowFlgCheck( meindex, talker, now);

		//何ページ  か
		page = CHAR_getWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC) ;
		
		if(page == 0 || page == 1){
			i = 0;
		}else{
			i = 83;
		}
		
		//ページ
		for(; i < nowindex ; i++)
		{
			sprintf(work,"%d,",now[ i]);
			strcat(work2,work);
		}
		
		sprintf(token,"�a�a�a�a�a「「�aチェックマン�a「「 "
					"\n�a�a現在の�v  の  っている NOWイベント"
					"\n%s"
			 	,work2);	
	  	
	  	if(page != 2 && nowindex > 83)
	  	{
	  		//モード
	  		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANT,1);
	  		//ページ
	  		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC,2);

			buttontype = WINDOW_BUTTONTYPE_NEXT;
			windowtype = WINDOW_MESSAGETYPE_MESSAGE;
		  	windowno = CHAR_WINDOWTYPE_CHECKMAN_MAIN; 

		}else{
			buttontype = WINDOW_BUTTONTYPE_OK;
			windowtype = WINDOW_MESSAGETYPE_MESSAGE;
		}
		
		break;


	//ENDフラグの簡単  示
	  case 2:
		//終  フラグのチェック
		nowindex = NPC_EndFlgCheck( meindex, talker, now);
		
		//何ページ  か
		page = CHAR_getWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC) ;
		
		if(page == 0 || page == 1){
			i = 0;
		}else{
			i = 83;
		}
		work2[0]=0;
		
		//ページ
		for(; i < nowindex ; i++)
		{
			sprintf(work,"%d,",now[ i]);
			strcat(work2,work);
		}
		sprintf(token,"�a�a�a�a�a「「�aチェックマン�a「「 "
					"\n�a�a現在の�v  の  っている ENDイベント"
					"\n%s"
			 	,work2);	
	  	
	  	if(page != 2 && nowindex > 83)
	  	{
	  		//モード
	  		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANT,2);
	  		//ページ
	  		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC,2);

			buttontype = WINDOW_BUTTONTYPE_NEXT;
			windowtype = WINDOW_MESSAGETYPE_MESSAGE;
		  	windowno = CHAR_WINDOWTYPE_CHECKMAN_MAIN; 

		}else{
			buttontype = WINDOW_BUTTONTYPE_OK;
			windowtype = WINDOW_MESSAGETYPE_MESSAGE;
		}
		
		break;
		
	
	// NOWフラグの詳細  示
	  case 4:
		{
			
			//NOWフラグチェック
			nowindex = NPC_NowFlgCheck( meindex, talker, now);

			if(NPC_FlgCheckMain( meindex, talker, nowindex,now,work2)
			 == FALSE)
			 {
			 	return;
			 }
			

			sprintf(token,"�a�a�a�a�a「「�aチェックマン�a「「 "
						"\n�a�a現在の�v  の  っている NOWイベント"
						"\n%s"
				 	,work2);	

			page = CHAR_getWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC) ;
		  	nowindex = (nowindex / ((6*page)+1));

		  	if(page != 16 &&  nowindex != 0)
		  	{
		  		//モード
		  		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANT,4);
		  		//ページ
		  		page = CHAR_getWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC);
		  		page++;
		  		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC,page);
			
				buttontype = WINDOW_BUTTONTYPE_NEXT;
				windowtype = WINDOW_MESSAGETYPE_MESSAGE;
			  	windowno = CHAR_WINDOWTYPE_CHECKMAN_MAIN; 
			}else{
				buttontype = WINDOW_BUTTONTYPE_OK;
				windowtype = WINDOW_MESSAGETYPE_MESSAGE;
			}

		}
	break;

	// ENDフラグの詳細  示
	  case 5:
		{
			//ENDフラグのチェック
			nowindex = NPC_EndFlgCheck( meindex, talker, now);
			
			
			if(NPC_FlgCheckMain( meindex, talker, nowindex, now, work2)
			 == FALSE)
			 {
			 	return;
			 }

			sprintf(token,"�a�a�a�a�a「「�aチェックマン�a「「 "
						"\n�a�a現在の�v  の  っている ENDイベント"
						"\n%s"
				 	,work2);	

			page = CHAR_getWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC) ;

		  	nowindex = (nowindex / ((6*page)+1));

		  	if(page != 16 &&  nowindex != 0)
		  	{
		  		//モード
		  		CHAR_setWorkInt(talker, CHAR_WORKSHOPRELEVANT, 5);
		  		//ページ
		  		page = CHAR_getWorkInt( talker, CHAR_WORKSHOPRELEVANTSEC);
		  		page++;
		  		CHAR_setWorkInt( talker, CHAR_WORKSHOPRELEVANTSEC, page);
			
				buttontype = WINDOW_BUTTONTYPE_NEXT;
				windowtype = WINDOW_MESSAGETYPE_MESSAGE;
			  	windowno = CHAR_WINDOWTYPE_CHECKMAN_MAIN; 
			}else{
				buttontype = WINDOW_BUTTONTYPE_OK;
				windowtype = WINDOW_MESSAGETYPE_MESSAGE;
			}

		}
	}
	
	makeEscapeString( token, escapedname, sizeof( escapedname));
	/*-ここで送信する--*/
	lssproto_WN_send( fd, windowtype, 
					buttontype, 
					windowno,
					CHAR_getWorkInt( meindex, CHAR_WORKOBJINDEX),
					escapedname);

}


/*-----------------------------------------
クライアントから返ってきた時に呼び出される�e
-------------------------------------------*/
void NPC_CheckManWindowTalked( int meindex, int talkerindex, 
								int seqno, int select, char *data)
{
	int datano;
	
	if( NPC_Util_CharDistance( talkerindex, meindex ) > 2) return;

	datano = atoi(data);
	
	if(select == WINDOW_BUTTONTYPE_OK) 
	{
		NPC_CheckMan_selectWindow( meindex, talkerindex, 0 );
	}else if(select == WINDOW_BUTTONTYPE_CANCEL) {
		return;
	}



	
	switch(CHAR_getWorkInt(talkerindex,CHAR_WORKSHOPRELEVANT)){
	  case 1:
	  	if(CHAR_getWorkInt(talkerindex,CHAR_WORKSHOPRELEVANTSEC) == 2){
			NPC_CheckMan_selectWindow( meindex, talkerindex, 1 );
	  	}
	  	break;

	  case 2:
	  	if(CHAR_getWorkInt(talkerindex,CHAR_WORKSHOPRELEVANTSEC) == 2){
			NPC_CheckMan_selectWindow( meindex, talkerindex, 2 );
	  	}
	  	break;

	  case 4:
	  	if(CHAR_getWorkInt(talkerindex,CHAR_WORKSHOPRELEVANTSEC) >= 2){
			NPC_CheckMan_selectWindow( meindex, talkerindex, 4 );
	  	}

	  case 5:
		if(CHAR_getWorkInt(talkerindex,CHAR_WORKSHOPRELEVANTSEC) >= 2){
			NPC_CheckMan_selectWindow( meindex, talkerindex, 5 );
		}

		break;
	}

	switch( datano ){
	  case 1:
			NPC_CheckMan_selectWindow( meindex, talkerindex, 1 );
		break;

	  case 2:
			NPC_CheckMan_selectWindow( meindex, talkerindex, 2 );
		break;

	  case 4:
			NPC_CheckMan_selectWindow( meindex, talkerindex, 4 );
		break;
		
	  case 5:
			NPC_CheckMan_selectWindow( meindex, talkerindex, 5 );
		break;
	}

}

/*
 *�aイベント  フラグをチェックする
 */
int NPC_NowFlgCheck(int meindex,int talker ,int nowflg[MAXEVENTFLG])
{
	int i = 0;
	int j = 0;
	
	for(i= 0; i < MAXEVENTFLG ; i++){
		if(NPC_NowEventCheckFlg( talker, i) == TRUE)
		{
			nowflg[j] = i;
			j++;
		}
 	}
 	
	return j;
}

/*
 *�aイベント終  フラグをチェックする
 */
int NPC_EndFlgCheck(int meindex,int talker ,int nowflg[MAXEVENTFLG])
{
	int i = 0;
	int j = 0;
	
	for(i= 0; i < MAXEVENTFLG ; i++){

		if(NPC_EventCheckFlg( talker, i) == TRUE)
		{
			nowflg[j] = i;
			j++;
		}
 	}
 	
	return j;
}

/*
 * 詳細
 *
 */
BOOL NPC_FlgCheckMain( int meindex,int talker,int nowindex,int now[MAXEVENTFLG],char *work2)
{
	int page;
	int max;
	int i;
	int shou;
	int j=1;
	char argstr1[NPC_UTIL_GETARGSTR_BUFSIZE];
	char *argstr;
	char work[512];
	char buf[40];
	char buf2[42];
	
	//何ページ  か
	page = CHAR_getWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC) ;

	if(page == 0) {
		page =1;
		CHAR_setWorkInt(talker,CHAR_WORKSHOPRELEVANTSEC,1) ;
	}

	if(page == 1){
		i = 0;
		if( nowindex >6 ){
			max =7;
		}else{
			max =nowindex;
		}
	}else{
		max = (page * 6) +1;
		i = max - 7;
		shou = nowindex / max;

		if(shou == 0){
			max = nowindex;
		}else{
			i = max - 7;
		}
	}
			
	//イベントの詳細が書かれているファイルを  みこむ
	if((argstr = NPC_Util_GetArgStr( meindex, argstr1, sizeof(argstr1))) == NULL) {
		print("NPC_CheckMan:GetArgStrErr");
		return FALSE;
	}
			
	work[0] = 0;
	work2[0] = 0;

	//ページ
	for(; i < max ; i++)
	{
		sprintf(work,"#%d:",now[ i]);
		j = 1;
		while( getStringFromIndexWithDelim(argstr, "|", j, buf,sizeof( buf))
		 !=FALSE )
		 {	
			j++;
			if(strstr(buf,work) != NULL){
				sprintf(buf2,"%s\n",buf);
				strcat(work2,buf2);
				break;
			}
		}
	}
	
	return TRUE;
}
