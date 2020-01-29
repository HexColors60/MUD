/*
 * cast a spell on someone or a monster
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef __linux__
 #include <netdb.h>
 #include <sys/socket.h>
 #include <sys/types.h> 
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
#endif

#ifdef _WIN32
 #include "winsock.h"
#endif

#include "defs.h"

extern user *users;
extern int allowplayerkilling;
extern char *maleusertitles[12];
extern char *femaleusertitles[12];
		
spell *spells=NULL;
char *spellconf[BUF_SIZE];
char *spellsrel="/config/spells.mud";

char *highlevelspell="That spell needs a higher level user to cast it\r\n";
char *nospellpoints="You don't have enough magic points to do that\r\n";
char *nospellhaven="You can't put spells on users in haven rooms\r\n";
char *notarget="No target for spell\r\n";
char *nospell="Spell not found\r\n";
char *spellhaskilled="and the spell has killed it!\r\n";

int castspell(user *currentuser,char *s,char *t) {
room *currentroom;
spell *spellnext;
user *usernext;
race *userrace;
monster *monsternext;
char *buf[BUF_SIZE];
int spellfound;
int pointsx;
int count;

if(t == NULL) {
 send(currentuser->handle,notarget,strlen(notarget),0);
 return;
}
currentroom=currentuser->roomptr;
userrace=currentuser->race;		/* get race */

/*
 * find spell
 */

 spellnext=spells;
 while(spellnext != NULL) {

  if(strcmp(spellnext->name,s) == 0) {			/* found spell */

   if((currentuser->status < spellnext->level) && currentuser->status < WIZARD) {   /* spell required a higher level user */
    send(currentuser->handle,highlevelspell,strlen(highlevelspell),0);
    return;
   }

/*
 * work out how many spell points are needed and display error message if not enough
 */

   if(currentuser->magicpoints-spellnext->spellpoints <= 0) {

	    if(currentuser->status < WIZARD) {
	     send(currentuser->handle,nospellpoints,strlen(nospellpoints),0);
	     return;
	    }
    }
  
    spellfound=TRUE;			/* found spell */
    break;
 }

 spellnext=spellnext->next;
}

if(spellfound == FALSE) {	/* no spell */
 send(currentuser->handle,nospell,strlen(nospell),0);
 return;
}

/*
 * casting spell on user
 */

 if((currentroom->attr & ROOM_HAVEN) == TRUE && currentuser->status < WIZARD) {		/* can't put spells on users in haven rooms */
  send(currentuser->handle,nospellhaven,strlen(nospellhaven),0);
  return;
 }
 
  usernext=users;
 
 while(usernext != NULL) {
   if(regexp(usernext->name,t) == TRUE) {		/* found target */

	 if(allowplayerkilling == FALSE) {		/* can't kill player */
  	  send(currentuser->handle,nospellhaven,strlen(nospellhaven),0);
	  return;
	 }

         if(usernext->status > WIZARD) {		/* if not user, cast spell */
	     if(usernext->gender == MALE) {
		     sprintf(buf,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->name,maleusertitles[currentuser->status]);
		     send(currentuser->handle,buf,strlen(buf),0);
		     return;
     	     } 
	     else
	     {
		     sprintf(buf,"%s casts a spell on %s the %s but it just bounces off with no effect\r\n",currentuser->name,femaleusertitles[currentuser->status]);
		     send(currentuser->handle,buf,strlen(buf),0);
		     return;
             }
       
        }

      pointsx=usernext->staminapoints-spellnext->damage;	/* deduct stamina points from user */

      updateuser(currentuser,usernext->name,"",0,0,"",0,pointsx,0,0,"","",0); /* update user stamina points */ 
      updateuser(currentuser,currentuser->name,"",0,0,"",usernext->magicpoints-spellnext->spellpoints,currentuser->staminapoints,0,0,"","",0); /* ' update own spell points */


      sprintf(buf,"%s casts a %s on %s causing %d points of damage\r\n",currentuser->name,spellnext->message,usernext->name,spellnext->damage);
      send(currentuser->handle,buf,strlen(buf),0);
  }

 usernext=usernext->next;
}

/*
 * cast spell on monster
 */

/* find monster */


for(count=0;count<currentroom->monstercount;count++) {
 if(regexp(t,currentroom->roommonsters[count].name) == TRUE) {		/* found object */

  /* calculate hit points */
  
   pointsx=spellnext->spellpoints / (currentuser->status/2) + userrace->strength+userrace->luck;
   monsternext->stamina -= pointsx;		/* deduct stamina points */

   sprintf(buf,"%s casts a %s on the %s and causes %d points of damage ",currentuser->name,spellnext->name,monsternext->name,pointsx);

   send(currentuser->handle,buf,strlen(buf),0);

   if(monsternext->stamina <= 0) {		/* monster has been killed */
    send(currentuser->handle,spellhaskilled,strlen(spellhaskilled),0);
   }
   else
   {
    send(currentuser->handle,"\r\n",2,0);
   }

   monsternext->last=monsternext->next;		/* remove monster */
   free(monsternext);
 }

}

return;
}

void loadspells(void) {
spell *spellnext;
FILE *handle;
char *b;
char c;
int lc;
char *ab[10][BUF_SIZE];
char *z[BUF_SIZE];
int errorcount=0;

getcwd(spellconf,BUF_SIZE);
strcat(spellconf,spellsrel);

 spellnext=spells;
 lc=0;

 handle=fopen(spellconf,"rb");
 if(handle == NULL) {                                           /* couldn't open file */
  printf("mud: Can't open configuration file %s\n",spellconf);
  exit(NOCONFIGFILE);
 }

 while(!feof(handle)) {
  fgets(z,BUF_SIZE,handle);		/* get and parse line */

  b=z;
  c=*b;
  if(c == '#')  continue;		/* skip comments */
  if(c == '\n')  continue;		/* skip newline */
 
  b=z;
  b=b+strlen(z);
  b--;

  if(*b == '\n') strtrunc(z,1);
  b--;
  if(*b == '\r') strtrunc(z,1);

  lc++;

  if(strlen(z) < 2) continue;		/* skip blank line */
  tokenize_line(z,ab,":\n");				/* tokenize line */

 if(strcmp(ab[0],"begin_spell") == 0) {	/* end */

   if(spells == NULL) {			/* first room */
    spells=calloc(1,sizeof(spell));
    if(spells == NULL) {
     perror("mud:");
     exit(NOMEM);
    }

    spellnext=spells;
   }
   else
   {
    spellnext->next=calloc(1,sizeof(spell));
    spellnext=spellnext->next;

    if(spellnext == NULL) {
     perror("mud:");
     exit(NOMEM);
    }
   }


   strcpy(spellnext->name,ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"spellpointsused") == 0) {	/* spell points used */
   spellnext->spellpoints=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"spelldamage") == 0) {	/* spell damage */
   spellnext->damage=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"spellmessage") == 0) {	/* spell message */
   strcpy(spellnext->message,ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"spelllevel") == 0) {		/* level  */
   spellnext->level=atoi(ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"spellclass") == 0) {		/* class */
   strcpy(spellnext->class,ab[1]);
   continue;			
  }

  if(strcmp(ab[0],"end") == 0) continue;

  if(strcmp(ab[0],"#") == 0) continue;			
 
  printf("mud: %d: uknown configuration option %s in %s\n",lc,ab[0],spellconf);		/* unknown configuration option */
  errorcount++;
}
 
fclose(handle);
}

