// Simulates whole rest of season
// (version before game-by-game version)

// Todo: tidy up local vs global pa[]
// and in general, tidy up globals
// Make adjustments work for any year
// (have a better way of storing all stuff for past years, including parsers)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#define EXPMODEL
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define MAXNAMES 1000 // Max number of team name (equiv classes)
#define MAXNEQ 100 // Max number of equiv team names
int neqn;// Number of equiv names
char *(eqname[MAXNAMES][MAXNEQ+1]);
char *nonsp(char *p){while(*p&&isspace(*p))p++;return p;}
char *getsp(char *p){while(*p&&!isspace(*p))p++;return p;}
char *col(int n,char *p){p=nonsp(p);while(n>0){p=nonsp(getsp(p));n--;}return p;}
#define PI (3.1415926535897932384)

#define NUML 5 // Number of leagues
#define MAXP 14 // 1+Max no. of properties
#define MAXREL 10 // Maximum number of teams, of which the probability of relegation for all
// possible subsets is calculated.
int prop[NUML][MAXP]={
  {300,350,400,500,550,600,1,2,3,4,6,-203,-1,0},
  {300,350,400,500,550,600,1,2,6,102,-203,-1,0},
  {300,350,400,500,550,600,1,2,6,102,-204,-1,0},
  {300,350,400,500,550,600,1,3,7,103,-202,-1,0},
  {300,350,400,500,550,600,1,5,101,-203,-1,0}
};
// n means top n
// -n means bottom n
// 100+n means top n + playoff(n+1,...,n+4) for promotion
// -200-n means bottom n for relegation
// 0 means termination
// 300 means luck points       \ These are guaranteed to be in first
// 350 means sd(luck points)   | six positions, for convenience.
// 400 means luck GD           |
// 500 means expected points   |
// 550 means sd(points)        |
// 600 means expected GD       /
// These points will turn into a spread index if the spread-index flag (sprind) is set
#define NUMSPR 6
int sprind1[NUMSPR]={60,40,30,20,10,5};

// Special adjustments

typedef struct {int d;char *t;int a;char *dt;} adjsdat; // Score adjustments (point deduction - relatively common)
typedef struct {int d;char *t0,*t1;int s0,s1;char *dt;} adjrdat; // Extra results    \ Very rare - used to make 2009-10
typedef struct {char *t,*dt;} adjedat;                           // Excluded teams   / work mid-season

typedef struct {
  int y;// year
  adjsdat *s;// See above
  adjrdat *r;// for
  adjedat *e;// description
  char *(mdt[NUML]); // Last match date for each league
  // Note that r,e are only likely to be used to overcome
  // deficiencies in the current year's results-scrape-site and probably
  // will not be needed in (cleaned) past years' results.
  // mdt mainly useful to prevent play-off results being counted as league results
  // (if a results parser didn't manage to exclude play-off results)
} adjalldat;

adjalldat adjall[]={
  {2003,
   (adjsdat[]){{4,"Burton_Albion",-1,"2003-11-26"},{0}},
   0,
   0,
   {"9999-12-31","9999-12-31","9999-12-31","9999-12-31","9999-12-31"}},

  {2004,
   (adjsdat[]){{2,"Wrexham",-10},{3,"Cambridge_Utd",-10,"2005-04-29"},{4,"Tamworth",-3},{4,"Northwich",-10},{0}},
   0,
   0,
   {"9999-12-31","9999-12-31","9999-12-31","9999-12-31","9999-12-31"}},

  {2005,
   (adjsdat[]){{4,"Altrincham",-18},{0}},
   0,
   0,
   {"9999-12-31","9999-12-31","9999-12-31","9999-12-31","9999-12-31"}},

  {2006,
   (adjsdat[]){{2,"Rotherham",-10},{4,"Crawley_Town",-10},{1,"Leeds_United",-10},{0}},
   0,
   0,
   {"2007-05-13","2007-05-06","2007-05-05","2007-05-05","2007-04-28"}},

  {2007,
   (adjsdat[]){{2,"Leeds_United",-15},{2,"Luton",-10},{2,"Bournemouth",-10},{3,"Rotherham",-10},
	       {4,"Halifax",-10},{4,"Crawley_Town",-6},{0}},
   0,
   0,
   {"2008-05-11","2008-05-04","2008-05-03","2008-05-03","2008-04-26"}},

  {2008,
   (adjsdat[]){
      {1,"Southampton",-10,"2009-04-23"},{2,"Stockport",-10},
      {3,"Rotherham",-17,0},{3,"Bournemouth",-17,0},{3,"Luton",-30,0},{3,"Darlington",-10,"2009-02-28"},
      {4,"Oxford_Utd",-5,0},{4,"Mansfield",-4,0},{4,"Crawley_Town",-1},{0}},
   // Southampton receiving the penalty in _this season_ was contingent on them finishing
   // above the relegation zone, which they didn't in fact do. But for the purposes of
   // relegation odds, it is better to pretend that they did receive the penalty in 2008-09.
   0,
   0,
   {"2009-05-24","2009-05-03","2009-05-02","2009-05-02","2009-04-26"}},

  {2009,
   (adjsdat[]){
      {0,"Portsmouth",-9,"2010-03-17"},{2,"Southampton",-10,"2009-06-01"},{4,"Salisbury",-10},
      {4,"Gateshead",-1},{1,"Crystal_Palace",-10},{0}},
   // Chester expelled from div=4 on 2010-03-08: still three others to be relegated
   // Nightmare to try to look at relegation betting for div4. In fact Forest Green got
   // a reprieve due to Salisbury getting kicked out in June for some infringement. How
   // long did people know this was on the cards? What did the bookies pay out on?
   0,
   0,
   {"2010-05-09","2010-05-02","2010-05-08","2010-05-08","2010-04-24"}},

  {2010,
   (adjsdat[]){
      {2,"Plymouth",-10,"2011-02-21"},
      {4,"Histon",-5,"2011-01-07"},
      {4,"Kidderminster",-5,"2011-01-07"},
      {0}},
   0,
   0,
   {"2011-05-22","2011-05-08","2011-05-07","2011-05-07","2011-04-30"}},

  {2011,
   (adjsdat[]){
      {1,"Portsmouth",-10,"2012-02-17"},
      {3,"Port_Vale",-10,"2012-03-06"},
      {4,"Darlington",-10,"2012-01-03"},
      {4,"Kettering",-3,"2012-02-29"},
      {0}},
   0,
   0,
   {"2012-05-13","2012-04-28","2012-05-05","2012-05-05","2012-04-20"}},

  {2012,
   (adjsdat[]){{0}},
   0,
   0,
   {"2013-05-19","2013-05-04","2013-04-27","2013-04-27","2013-04-21"}},

  {2013,
   (adjsdat[]){
    {2,"Coventry",-10,"2013-08-02"},
    {4,"Aldershot",-10,"2013-06-08"},
    {0}},
   0,
   0,
   {"2014-05-12","2014-05-04","2014-05-04","2014-05-04","2014-04-27"}},

  {0,// Special terminating entry that creates no adjustments
   (adjsdat[]){{0}},
   0,
   0,
   {"9999-12-31","9999-12-31","9999-12-31","9999-12-31","9999-12-31"}}
};

adjalldat *aa;

/*
  Old adjustments

  2009-10
struct {int d;char *t0,*t1;int s0,s1;char *dt;} adjr[NUMRA]={
  {4,"Gateshead","Hayes_&_Yeading",0,0,"2009-09-05"}
};
#define NUMET 1 // Number of excluded teams
char *(exclt[NUMET])={"Chester"};
char *(maxdt[NUML])={"2010-05-09","2010-05-02","2010-05-08","2010-05-08","2010-04-24"};
*/

double drnd(){return (random()+.5)/(RAND_MAX+1.0);}

unsigned int secs(char *s){
  // This does what strptime should do, but strptime doesn't properly exist
  // E.g., "2003-11-19" --> seconds since 1970
  unsigned int m,y;
  unsigned int mm[12]={0,31,59,90,120,151,181,212,243,273,304,334};
  y=atoi(s);if(y<1970||y>2106)return 0;
  m=atoi(s+5)-1;
  return ((y-1970)*365+(y-1969)/4+mm[m]+(m>=2&&(y&3)==0)+atoi(s+8)-1)*24*60*60;
}

int update,ln,maxit,steps,year,useres,rel,sprind,dohtml,eval,prl;
char datadir[1000],divname[1000],cutofftime[100];
struct tm now;// Program start time in GMT
double acc,rej;
double trialsd=0.04;

void getname(int year,int div,char *divname){
  char *(dn0[5])={"Premiership","Division 1","Division 2","Division 3","Conference"};
  char *(dn1[5])={"Premiership","Championship","League 1","League 2","Conference"};
  char *(dn2[5])={"Premier League","Championship","League 1","League 2","Conference"};
  if(year<2004)strcpy(divname,dn0[div]); else
    if(year<2007)strcpy(divname,dn1[div]); else
      strcpy(divname,dn2[div]);
}

void initargs(int ac,char **av){
  int i,seed;
  char *l,*date;
  time_t t0;
  struct tm tm;
  FILE *fp;
  
  t0=time(NULL);seed=8716283*t0+81123871*getpid()+7128973;
  update=0;ln=-1;maxit=20000;steps=10;date=0;useres=99999;rel=0;
  sprind=0;dohtml=0;eval=0;prl=1;
  for(i=1;i<ac;i++)if((strlen(av[i])>=4&&strncmp(av[i]+strlen(av[i])-4,"help",4)==0)
		      ||strncmp(av[i],"?",1)==0){
    printf("Help message\n");exit(0);
  }
  for(i=1;i<ac;i++){
    int nn,rr;
    char *l0;
    l=av[i];l0=strchr(l,'=');
    if(l0){
      l0++;rr=l0-l;nn=atoi(l0);
      if(!strncmp(l,"update=",rr)){update=nn;continue;}
      if(!strncmp(l,"div=",rr)){ln=nn;continue;}// league number 0-4 or -1 for all
      if(!strncmp(l,"eval=",rr)){eval=nn;continue;}// eval mode
      if(!strncmp(l,"maxit=",rr)){maxit=nn;continue;}
      if(!strncmp(l,"steps=",rr)){steps=nn;continue;}
      if(!strncmp(l,"seed=",rr)){seed=nn;continue;}
      if(!strncmp(l,"sprind=",rr)){sprind=nn;continue;}
      if(!strncmp(l,"prl=",rr)){prl=nn;continue;}//print level
      if(!strncmp(l,"dohtml=",rr)){dohtml=nn;continue;}
      if(!strncmp(l,"useres=",rr)){useres=nn;continue;}
      if(!strncmp(l,"date=",rr)){date=strdup(l0);continue;}
      // ^ Either (i) omitted, (ii) a specific YYYY-MM-DD date,
      //   or (iii) a year (e.g., 2006 represents the 2006-07 season)
      if(!strncmp(l,"rel=",rr)){rel=nn;continue;}// Work out probabilities of subsets of 'rel' from the bottom
      fprintf(stderr,"Can't parse argument \"%s\"\n",l);exit(1);
    }
  }
  if(rel>MAXREL){fprintf(stderr,"rel too large. MAXREL=%d\n",MAXREL);exit(1);}
  fp=fopen("logfile","a");if(fp==0){printf("Couldn't open log file\n");exit(0);}
  l=ctime(&t0);for(i=0;l[i];i++)if(l[i]!='\n')fprintf(fp,"%c",l[i]);
  fprintf(fp,"  seed=%d pid=%d : ",seed,getpid());
  for(i=0;i<ac;i++)fprintf(fp,"%s ",av[i]);fprintf(fp,"\n");fclose(fp);
  localtime_r(&t0,&tm);
  strcpy(cutofftime,"9999-12-31");
  if(date==0)year=tm.tm_year+1900-(tm.tm_mon<7);                   // Date not specified
  else if(strlen(date)==4)year=atoi(date);                         // E.g., "2006" for 2006-07 season
  else {year=atoi(date)-(atoi(date+5)<7);strcpy(cutofftime,date);} // E.g., "2007-01-23" for 2006-07 season up to 2007-01-23T23:59:59
                                                                   // or    "2007-01-23T12:34:56" for 2006-07 season up to 2007-01-23T12:34:56
  if(strlen(cutofftime)==10)strcat(cutofftime,"T23:59:59");
  printf("Season %d-%d, taking matches up to %s\n",year,year+1,cutofftime);
  srandom(seed);srand48(seed);
}

#define MAXNT 100 // Maximum number of teams in any particular league
#define MAXTL 100 // 1+Maximum length of team description string
#define MAXNM 10000 // Maximum number of matches (rounds)
#define MAXS 100 // 1+Maximum number of goals scored
int nr,nt,np,res[MAXNM][5],maxtl,rels[1<<MAXREL];
long long int stat[MAXP][MAXNT];
double odds[MAXNM][3];// NCU
char tm[MAXNT][MAXTL];
int last;
double lfac[MAXS];
double phg[MAXS],pag[MAXS];
int rell[MAXNT]; // List of teams in the relegation set
int relm[MAXNT]; // Left inverse to rell[] (returns -1 if team is not in rel. set)
double relp[MAXNT];

void loadequivnames(){
  int n,s,t;
  FILE *fp;
  char l[10000],*l1,*l2;
  fp=fopen("equivnames","r");
  t=0;
  while(fgets(l,10000,fp)){
    assert(t<MAXNAMES);
    s=strlen(l);assert(s>0);l[s-1]=' ';
    n=0;l1=l;
    while(1){
      l2=strchr(l1,' ');if(!l2)break;
      assert(l2-l1<MAXTL);assert(n<MAXNEQ);
      l2[0]=0;eqname[t][n]=strdup(l1);
      n++;l1=l2+1;
    }
    eqname[t][n]=0;
    t++;
  }
  neqn=t;
}

char *eqnormalise(char *s){
  int i,j;
  for(i=0;i<neqn;i++)
    for(j=0;eqname[i][j];j++)
      if(strcmp(s,eqname[i][j])==0)return eqname[i][0];
  fprintf(stderr,"Team %s not found in equiv name list\n",s);exit(1);
}

int s2n(char *s,int add){// 'add' = whether to add in new names
  int i,sl;
  char l0[MAXTL],*l1;
  strcpy(l0,s);
  while(1){
    l1=strstr(l0,"&amp;");if(!l1)break;
    memmove(l1+1,l1+5,strlen(l1+5)+1);
  }
  while(1){
    l1=strchr(l0,' ');if(!l1)break;
    l1[0]='_';
  }
  l1=eqnormalise(l0);
  if(aa->e)for(i=0;aa->e[i].t;i++)if(strcmp(cutofftime,aa->e[i].dt)>=0&&strcmp(l1,aa->e[i].t)==0)return -1;
  // ^ check excluded list: reject if excluded by the specified date
  for(i=0;i<nt;i++)if(strcmp(l1,tm[i])==0)return i;
  if(!add)return -1;
  assert(nt<MAXNT);
  sl=strlen(l1);assert(sl<MAXTL);if(sl>maxtl)maxtl=sl;
  strcpy(tm[i],l1);
  nt++;return i;
}

#ifdef EXPMODEL
double fn(double x){return exp(x);}
double fn1(double x){return exp(x);}// fn'(x)
double fn2(double x){return exp(x);}// fn''(x) (Currently unused)
#else
double fn(double x){return x+2;}
double fn1(double x){return 1;}// fn'(x)
double fn2(double x){return 0;}// fn''(x)
#endif

/*
double fn(double x){return 3.5*exp(x)/(exp(x)+1);}
double fn1(double x){return 3.5*exp(x)/((exp(x)+1)*(exp(x)+1));}// fn'(x)
double fn2(double x){return 0;}// fn''(x)
*/

double poff[NUML]={ 1.6, 1.2, .9, .6, 0.2 };// strength offsets for each division
double ppa[2*MAXNT+2],ppaiv[2*MAXNT+2];// iv = inverse variance
double pa[2*MAXNT+2],*al,*be,*hh;
int it,pg[MAXNT],sc[MAXNT],scu[MAXNT],gd[MAXNT],gdu[MAXNT],gs[MAXNT];

double prior(double *lpa,double *L1){
  int t;
  double x,A,B,C,w1,w2,L,s2,det;
  // Ignore constant terms .5*log(ppaiv[t]/(2*PI)) etc to make it easier to use ppaiv[t]=0
  // This will make information printing ("bits") a little out
  for(t=0,s2=0;t<2*nt+2;t++)s2+=ppaiv[t]*pow(lpa[t]-ppa[t],2);
  x=ppaiv[2*nt]*(lpa[2*nt]-ppa[2*nt])-ppaiv[2*nt+1]*(lpa[2*nt+1]-ppa[2*nt+1]);
  for(t=0,w1=-x;t<nt;t++)w1+=ppaiv[t]*(lpa[t]-ppa[t]);
  for(t=nt,w2=x;t<2*nt;t++)w2+=ppaiv[t]*(lpa[t]-ppa[t]);
  B=ppaiv[2*nt]+ppaiv[2*nt+1];
  for(t=0,A=B;t<nt;t++)A+=ppaiv[t];
  for(t=nt,C=B;t<2*nt;t++)C+=ppaiv[t];
  det=A*C-B*B;
  L=-.5*(s2-(C*w1*w1+2*B*w1*w2+A*w2*w2)/det);
  if(L1){
    for(t=0;t<2*nt+2;t++)L1[t]=-ppaiv[t]*(lpa[t]-ppa[t]);
    for(t=0;t<nt;t++)L1[t]+=(C*ppaiv[t]*w1+B*ppaiv[t]*w2)/det;
    for(t=nt;t<2*nt;t++)L1[t]+=(A*ppaiv[t]*w2+B*ppaiv[t]*w1)/det;
    L1[2*nt]+=(-C*w1+B*(w1-w2)+A*w2)*ppaiv[2*nt]/det;
    L1[2*nt+1]+=(C*w1+B*(w2-w1)-A*w2)*ppaiv[2*nt+1]/det;
  }
  return L;
}

double getL(double *L1){
  int a,b,c,d,i,r;
  double L,la,la1,mu,mu1,d1,d2;
  L=prior(pa,L1);
  for(r=0;r<nr;r++){
    a=res[r][0];b=res[r][1];c=res[r][2];d=res[r][3];
    la=fn(al[a]-be[b]+hh[0]);
    la1=fn1(al[a]-be[b]+hh[0]);
    //la2=fn2(al[a]-be[b]+hh[0]);
    mu=fn(al[b]-be[a]-hh[1]);
    mu1=fn1(al[b]-be[a]-hh[1]);
    //mu2=fn2(al[b]-be[a]-hh[1]);
    L+=-la+c*log(la)-lfac[c]-mu+d*log(mu)-lfac[d];
    //L+=-log(phg[c])-log(pag[d]);
    //if(it==5000)printf("%12g --> %d\n%12 --> %d\n",la,c,mu,d);
    L1[a]+=(-1+c/la)*la1;
    L1[b]+=(-1+d/mu)*mu1;
    L1[nt+a]-=(-1+d/mu)*mu1;
    L1[nt+b]-=(-1+c/la)*la1;
    L1[2*nt]+=(-1+c/la)*la1;
    L1[2*nt+1]-=(-1+d/mu)*mu1;
  }
  d1=d2=0;
  for(i=0;i<nt;i++){d1+=L1[i];d2+=L1[nt+i];}
  for(i=0;i<nt;i++){L1[i]-=d1/nt;L1[nt+i]-=d2/nt;}
  return L;
}

/*
  // Derivative test (need to take eps=0)
  for(i=0;i<np;i++)pa[i]=drnd()-.5;
  L=getL(L1);
  for(i=0;i<np;i++)tt[i]=L1[i];
  for(i=0;i<np;i++){
    pa[i]+=del;x=getL(L1);
    pa[i]-=2*del;x-=getL(L1);
    pa[i]+=del;
    x/=2*del;
    printf("%12g %12g\n",tt[i],x);
  }
  exit(0);
*/

double getL0(double *pa){
  int a,b,c,d,r;
  double L,la,mu,*al,*be,*hh;
  al=pa;be=pa+nt;hh=pa+2*nt;
  L=prior(pa,0);
  for(r=0;r<nr;r++){
    a=res[r][0];b=res[r][1];c=res[r][2];d=res[r][3];
    la=fn(al[a]-be[b]+hh[0]);
    mu=fn(al[b]-be[a]-hh[1]);
    L+=-la+c*log(la)-lfac[c]-mu+d*log(mu)-lfac[d];
  }
  return L;
}

void prparams(double *pa){
  int i;
  double L,*al,*be,*hh;
  al=pa;be=pa+nt;hh=pa+2*nt;
  L=getL0(pa);
  printf("\nit %d\nL %10g bits      L/nr %10g bits\n",it,L/log(2),L/log(2)/nr);
  for(i=0;i<nt;i++)printf("%-*s  %12g  %12g\n",maxtl,tm[i],al[i],be[i]);
  printf("%12g\n%12g\n",hh[0],hh[1]);
}

void opt(void){// Find MLE
  int i,end;
  double x,del,L,L1[np];
  for(i=0;i<np;i++)pa[i]=0;
  del=1e-3;
  it=0;end=0;
  while(1){
    L=getL(L1);
    for(i=0,x=0;i<np;i++)x+=L1[i]*L1[i];x=sqrt(x/np);
    if(end||it%1000==0){
      printf("\nit %d of MLE-finding\nL %10g bits      L/nr %10g bits\n",it,L/log(2),L/log(2)/nr);
      for(i=0;i<nt;i++)printf("%-*s  %12g  %12g  %12g  %12g\n",maxtl,tm[i],al[i],be[i],L1[i],L1[nt+i]);
      printf("%12g  %12g\n%12g  %12g\n",hh[0],L1[2*nt],hh[1],L1[2*nt+1]);
    }
    if(end)break;
    if(x<1e-9||it==50000)end=1;
    for(i=0;i<np;i++)pa[i]+=del*L1[i];// pa+=delta*L' = crude gradient descent
    it++;
  }
}

double norm(void){
  static int s=0;
  static double r,th;
  if(s==0){
    r=sqrt(-2*log(drnd()));th=2*PI*drnd();
    s=1;
    return r*cos(th);
  }
  s=0;
  return r*sin(th);
}

int getpois(double la){
  int i;
  double x,y;
  if(la>500){
    //printf("%g ",la);fflush(stdout);
    i=floor(norm()*sqrt(la)+la+.5);if(i<0)i=0;
    return i;
  }
  x=drnd()-1e-10;
  y=exp(-la);
  i=0;while(1){
    x-=y;if(x<0)return i;
    i++;
    y*=la/i;
  }
}

void getsc(int *h,int *a,int i,int j){// Simulate team i vs team j, putting results in *h, *a
  double la,mu;
  la=fn(al[i]-be[j]+hh[0]);
  mu=fn(al[j]-be[i]-hh[1]);
  *h=getpois(la);
  *a=getpois(mu);
}

int semi(int t0,int t1){
  int i,s0,s1,x0,x1;
  double la,mu;
  getsc(&s1,&s0,t1,t0);// Two legs. Lower placed team is home first
  getsc(&x0,&x1,t0,t1);// (which affects silver goal)
  s0+=x0;s1+=x1;
  if(s0>s1)return t0;
  if(s0<s1)return t1;
  la=fn(al[t0]-be[t1]+hh[0])*15/90.;// Silver goal : two periods of 15 minutes
  mu=fn(al[t1]-be[t0]-hh[1])*15/90.;
  for(i=0;i<2;i++){
    s0=getpois(la);s1=getpois(mu);
    if(s0>s1)return t0;
    if(s0<s1)return t1;
  }
  // Still tied - penalty shoot out. Assume random.
  if(drnd()<.5)return t0; else return t1;
}

int final(int t0,int t1){
  int i,s0,s1;
  double ha,la,mu;
  ha=(hh[0]-hh[1])/2;// Single leg played on neutral ground
  la=fn(al[t0]-be[t1]+ha);
  mu=fn(al[t1]-be[t0]+ha);
  for(i=0;i<3;i++){// 90 mins + Silver goal = two periods of 15 minutes
    s0=getpois(la);s1=getpois(mu);
    if(s0>s1)return t0;
    if(s0<s1)return t1;
    if(i==0){
      la*=15/90.;mu*=15/90.;
    }
  }
  // Still tied - penalty shoot out. Assume random.
  if(drnd()<.5)return t0; else return t1;
}

int playoff(int a,int b,int c,int d){
  return final(semi(a,d),semi(b,c));
}

char *descp(int p){// not re-entrant (only use once at a time)
  static char l[100];
  if(p==300)return "Points";  //
  if(p==350)return "%(pts)";  // Luck
  if(p==400)return "GD";      // 
  if(p==500)return sprind?"Index":"Points";    //
  if(p==550)return sprind?"sd(ind)":"sd(pts)"; // Expected
  if(p==600)return "GD";                       //
  if(p>0&&p<100){sprintf(l,"Top%d",p);return l;}
  if(p<0&&p>-100){sprintf(l,"Bot%d",-p);return l;}
  if(p>100&&p<200){sprintf(l,"%d+[%d-%d]",p-100,p-99,p-96);return l;}
  if(p<-200&&p>-300){sprintf(l,"Bot%d",-200-p);return l;}
  fprintf(stderr,"Unrecognised property %d\n",p);exit(1);
  return 0;
}

void getnewparams(void){
  int i,j;
  double x,ml,nl,pa1[2*MAXNT+2];
  if(steps==0)return;
  ml=getL0(pa);
  for(i=0;i<steps;i++){
    for(j=0;j<2*nt+2;j++)pa1[j]=pa[j]+trialsd*norm();
    for(j=0,x=0;j<nt;j++)x+=pa1[j];x/=nt;for(j=0;j<nt;j++)pa1[j]-=x;
    for(j=0,x=0;j<nt;j++)x+=pa1[nt+j];x/=nt;for(j=0;j<nt;j++)pa1[nt+j]-=x;
    nl=getL0(pa1);
    //for(j=0,x=0;j<2*nt+2;j++)x+=(pa1[j]-pa[j])*(pa1[j]-pa[j]);
    //-x/(2*trialsd*trialsd)-(2*nt+2)*log(trialsd*sqrt(2*PI))
    if(drnd()<exp(nl-ml)){
      memcpy(pa,pa1,(2*nt+2)*sizeof(double));
      ml=nl;
      acc++;
      //if(it==9511){printf("Step %d\n",i);prparams(pa);}
    }else rej++;
  }
}

double pt[MAXNT];
int cmp(const void*x,const void*y){
  double z;
  z=pt[*((int*)y)]-pt[*((int*)x)];
  return (z>0?1:(z<0?-1:0));
}
int cmps(const void*x,const void*y){
  int a,b,d;
  a=*((int*)y);b=*((int*)x);
  d=sc[a]-sc[b];if(d!=0)return d;
  d=gd[a]-gd[b];if(d!=0)return d;
  d=gs[a]-gs[b];if(d!=0)return d;
  return strcmp(tm[b],tm[a]);
}
int cmpres(const void*x,const void*y){// compares results by date
  int *a,*b;
  a=*((int(*)[5])x);b=*((int(*)[5])y);
  return a[4]-b[4];
}
#define SGN(x) ((x)>0?1:((x)<0?-1:0))
void checkres(void){
  int i,j,r,t0,t1,ind[nt][nt];
  for(i=0;i<nt;i++)for(j=0;j<nt;j++)ind[i][j]=-1;
  for(r=0;r<nr;r++){
    t0=res[r][0];t1=res[r][1];
    i=ind[t0][t1];
    if(i>=0){
      if(res[r][2]==res[i][2]&&res[r][3]==res[i][3]&&res[r][4]==res[i][4]){
	fprintf(stderr,"Warning: %s vs %s listed twice (same result)\n",tm[t0],tm[t1]);
	memmove(res[r],res[r+1],(nr-1-r)*5*sizeof(int));
	r--;nr--;
      }else{
	fprintf(stderr,"Error: %s vs %s listed twice (different result)\n",tm[t0],tm[t1]);
	exit(1);
      }
    }else ind[t0][t1]=r;
  }
}
void sim(int cl){
  int a,c,h,i,j,k,n,p,r,t,fl,pl[nt][nt],ord[nt],sc0[nt],sc1[nt],gd0[nt],gd1[nt],gs1[nt];
  char l[100];
  double x,y;
  FILE *fpo;
  for(i=0;i<nt;i++)for(j=0;j<nt;j++)pl[i][j]=0;
  for(r=0;r<nr;r++)pl[res[r][0]][res[r][1]]++;
  for(i=0;i<MAXP;i++)for(j=0;j<nt;j++)stat[i][j]=0;
  for(i=0;i<(1<<rel);i++)rels[i]=0;
  for(i=0;i<nt;i++)ord[i]=i;
  for(it=0;it<=maxit;it++){
    if(it%(MAX(maxit/10,1))==0||it==maxit){//print
      for(k=0;k<1+(it==maxit)+(it==maxit&&nr==nt*(nt-1));k++){
        char tbuf[100];
	if(k==0){printf("\n");fpo=stdout;} else {
          if(k==1)sprintf(l,"%s/table",datadir); else sprintf(l,"%s/finaltable",datadir);
	  fpo=fopen(l,"w");assert(fpo);
	}
        strftime(tbuf,1000,"%Y-%m-%dT%H:%M:%S",&now);
        fprintf(fpo,"ASOF         %s+0000\n",cutofftime);
        fprintf(fpo,"UPDATED      %s+0000\n",tbuf);
        if(last>=0){
          time_t t0;
          struct tm tt0;
          t0=res[last][4];assert(gmtime_r(&t0,&tt0));strftime(tbuf,1000,"%Y-%m-%d",&tt0);
          fprintf(fpo,"LASTPLAYED   %s %s %s %d %d\n",tbuf,tm[res[last][0]],tm[res[last][1]],res[last][2],res[last][3]);
        }else fprintf(fpo,"LASTPLAYED   none played so far\n");
	fprintf(fpo,"ITERATIONS   %d\n",it);
	if(k==0)fprintf(fpo,"acc %.1f%%    acc%%*sd^2 %g\n",100*acc/(acc+rej),100*acc/(acc+rej)*trialsd*trialsd);
	//for(i=0;i<nt;i++)pt[i]=stat[3][i]+stat[5][i]/1000.;
	//for(i=0;i<nt;i++)pt[i]=sc[i]*1e5+gd[i];
	qsort(ord,nt,sizeof(int),cmps);
	fprintf(fpo,"BEGINTABLE\n");
	fprintf(fpo,"%-*s  -- Current --     --------- Luck ---------   ------- Expected -------     ",maxtl,"");
	for(i=0;i<MAXP&&prop[cl][i];i++);
	j=9*(i-2)-60;
	for(t=0;t<j/2;t++)fprintf(fpo,"-");
	fprintf(fpo," Percentage chance ");
	for(t=0;t<(j+1)/2;t++)fprintf(fpo,"-");
	fprintf(fpo,"\n");
	fprintf(fpo,"%-*s  Pld  Pts   GD  ",maxtl,"Team");
	for(i=0;i<MAXP&&prop[cl][i];i++)fprintf(fpo,"%9s",descp(prop[cl][i]));fprintf(fpo,"\n");
	for(i=0;i<nt;i++){
	  t=ord[i];
	  fprintf(fpo,"%-*s   %2d  %3d %+4d    ",maxtl,tm[t],pg[t],sc[t],gd[t]);
	  for(j=0;j<MAXP;j++){
	    p=prop[cl][j];if(!p)break;
	    c=' ';if(stat[j][t]>0){
	      if(p>100&&p<200)c='P';
	      if(p<-200)c='R';
	    }
	    x=stat[j][t];
	    if(j==4){y=stat[3][t];x=sqrt((it*x-y*y)/(it*(it-1.)));} else x/=it;
	    fprintf(fpo,"%7.2f%c ",x*(p<300?100:1),c);
	  }
	  fprintf(fpo,"\n");
	}
	for(i=0,fl=0;aa->s[i].t;i++)if(aa->s[i].d==cl)fl=1;
	if(aa->r)for(i=0;aa->r[i].t0;i++)if(aa->r[i].d==cl)fl=1;
	if(fl){
	  fprintf(fpo,"\nAdjustments:\n");
	  if(aa->r)for(i=0;aa->r[i].t0;i++)if(aa->r[i].d==cl)
		   fprintf(fpo,"%-*s   %d - %d   %-*s\n",
			   maxtl,aa->r[i].t0,aa->r[i].s0,aa->r[i].s1,maxtl,aa->r[i].t1);
	  for(i=0;aa->s[i].t;i++)if(aa->s[i].d==cl)fprintf(fpo,"%-*s   %d\n",maxtl,aa->s[i].t,aa->s[i].a);
	}
	if(rel){
	  for(t=0;t<rel;t++)relp[t]=0;
	  for(n=0;n<(1<<rel);n++)for(t=0;t<rel;t++)if(n&(1<<t))relp[t]+=rels[n];
	  for(n=0;n<(1<<rel);n++){
	    for(t=0;t<rel;t++)fprintf(fpo," %s(%s)",(n&(1<<t))?"REL":" UP",tm[rell[t]]);
	    for(t=0,x=1;t<rel;t++){y=relp[t]/(double)it;if(n&(1<<t))x*=y; else x*=1-y;}
	    fprintf(fpo,"    %6.2f%%   %6.2f%%\n",rels[n]*100/(double)it,x*100);
	  }
	}
	if(k>0)fclose(fpo);
      }//k
    }// if print
    getnewparams();
    //if(it==13230)prparams(pa);
    for(i=0;i<nt;i++){sc1[i]=sc[i];sc0[i]=scu[i];gd1[i]=gd[i];gd0[i]=gdu[i];gs1[i]=gs[i];}
    for(i=0;i<nt;i++)for(j=0;j<nt;j++)if(i!=j){
      getsc(&h,&a,i,j);
      if(pl[i][j]){// Game has been played: work out contribution to luck
        sc0[i]-=(h>a?3:(h==a?1:0));
        sc0[j]-=(a>h?3:(h==a?1:0));
        gd0[i]-=h-a;gd0[j]-=a-h;
      }else{// Game hasn't been played: work out contribution to prediction
        //if(abs(h)>1000000000||abs(a)>1000000000)printf("it %d  %d.%s vs %d.%s -> %d %d\n",it,i,tm[i],j,tm[j],h,a);
        sc1[i]+=(h>a?3:(h==a?1:0));
        sc1[j]+=(a>h?3:(h==a?1:0));
        gd1[i]+=h-a;gd1[j]+=a-h;
        gs1[i]+=h;gs1[j]+=a;
      }
    }
    for(i=0;i<nt;i++)pt[i]=sc1[i]*1e8+gd1[i]*1e4+gs1[i]+.1*drnd();
    qsort(ord,nt,sizeof(int),cmp);// now ord[] maps final position to team number
    if(sprind)for(i=0;i<NUMSPR;i++){
      stat[3][ord[i]]+=sprind1[i];
      stat[4][ord[i]]+=sprind1[i]*sprind1[i];
    }
    for(i=0;i<nt;i++){
      stat[0][i]+=sc0[i];
      stat[1][i]+=100*((sc0[i]>0)+.5*(sc0[i]==0));
      stat[2][i]+=gd0[i];
      if(!sprind){
        stat[3][i]+=sc1[i];
        stat[4][i]+=sc1[i]*sc1[i];
      }
      stat[5][i]+=gd1[i];
    }
    for(i=6;i<MAXP&&prop[cl][i];i++){
      p=prop[cl][i];
      if(p>0&&p<100){for(j=0;j<MIN(p,nt);j++)stat[i][ord[j]]++;continue;}
      if(p<0&&p>-100){for(j=MAX(nt+p,0);j<nt;j++)stat[i][ord[j]]++;continue;}
      if(p>100&&p<200){
	p-=100;
	for(j=0;j<MIN(p,nt);j++)stat[i][ord[j]]++;
	stat[i][playoff(ord[p],ord[p+1],ord[p+2],ord[p+3])]++;
	continue;
      }
      if(p<-200&&p>-300){
	p+=200;for(j=MAX(nt+p,0);j<nt;j++)stat[i][ord[j]]++;
	if(rel){
	  for(j=MAX(nt+p,0),n=0;j<nt;j++){k=relm[ord[j]];if(k>=0)n|=1<<k;}
	  assert(n>=0&&n<(1<<rel));rels[n]++;
	}
	continue;
      }
      fprintf(stderr,"Unrecognised property %d\n",p);exit(1);
    }
  }
}

double poisent(double la){
  int r;
  double m,x,ll;
  m=exp(-la);x=0;ll=log(la);
  for(r=0;r<MAXS;r++){
    x+=m*(la-r*ll+lfac[r]);
    m*=la/(r+1);
  }
  return x;
}

void doeval(){
  int a,b,c,d,i,nr0;
  double x,la,mu,pr,tp,del,L,L1[np];
  for(i=0;i<np;i++)pa[i]=0;
  del=2e-3;
  nr0=nr;
  printf("%d results\n",nr0);
  for(nr=200,tp=0;nr<nr0;nr++){
    it=0;
    while(1){
      L=getL(L1);
      for(i=0,x=0;i<np;i++)x+=L1[i]*L1[i];x=sqrt(x/np);
      if(x<1e-6)break;
      if(0&&it%1000==0){
	printf("\nit %d\nL %10g bits      L/nr %10g bits\n",it,L/log(2),L/log(2)/nr);
	for(i=0;i<nt;i++)printf("%-*s  %12g  %12g  %12g  %12g\n",maxtl,tm[i],al[i],be[i],L1[i],L1[nt+i]);
	printf("%12g  %12g\n%12g  %12g\n",hh[0],L1[2*nt],hh[1],L1[2*nt+1]);
      }
      for(i=0;i<np;i++)pa[i]+=del*L1[i];
      it++;
      if(it==50000){fprintf(stderr,"Couldn't find MLE at nr=%d\n",nr);exit(1);}
    }
    a=res[nr][0];b=res[nr][1];c=res[nr][2];d=res[nr][3];
    la=fn(al[a]-be[b]+hh[0]);
    mu=fn(al[b]-be[a]-hh[1]);
    pr=-la+c*log(la)-lfac[c]-mu+d*log(mu)-lfac[d];
    printf("%4d %4d %7.3f %*s %*s %2d %2d\n",nr,it,pr,maxtl,tm[a],maxtl,tm[b],c,d);
    tp+=pr;
  }
  printf("Total likelihood %.3f\n",tp);

  for(nr=200,tp=0;nr<nr0;nr++){
    a=res[nr][0];b=res[nr][1];c=res[nr][2];d=res[nr][3];
    la=fn(al[a]-be[b]+hh[0]);
    mu=fn(al[b]-be[a]-hh[1]);
    pr=-la+c*log(la)-lfac[c]-mu+d*log(mu)-lfac[d];
    //printf("%4d %4d %7.3f %*s %*s %2d %2d\n",nr,it,pr,maxtl,tm[a],maxtl,tm[b],c,d);
    tp+=pr;
  }
  printf("Total likelihood %.3f\n",tp);

}

void getpriorparams(int cl0){
  int i,t,nf,cl,pln[MAXNT];
  double x,y;
  char l[1000],*l1;
  FILE *fp;
  for(i=0;i<nt;i++){
    pln[i]=-1;
    if(cl0<4)ppa[i]=ppa[nt+i]=0; else ppa[i]=ppa[nt+i]=-.2;
    // ^ A previously unseen team entering the Conference is likely to be worse than average
    ppaiv[i]=ppaiv[nt+i]=20;
    // ^ A previously unseen team is more unknown than a previously seen team
  }
  ppa[2*nt]=0.35;ppaiv[2*nt]=25;
  ppa[2*nt+1]=0;ppaiv[2*nt+1]=25;
  nf=0;
  for(cl=0;cl<NUML;cl++){// Scan all of last year's leagues to find teams in the current league
    sprintf(l,"data/%02d%02d/div%d/finalratings",(year-1)%100,year%100,cl);
    fp=fopen(l,"r");if(!fp){fprintf(stderr,"Warning: Couldn't open prev finalratings file %s\n",l);continue;}
    while(fgets(l,1000,fp)){
      l1=getsp(l);
      if(cl==cl0&&l1-l==7&&strncmp(l,"HOMEADV",l1-l)==0)ppa[2*nt]=atof(l1);
      if(cl==cl0&&l1-l==10&&strncmp(l,"AWAYDISADV",l1-l)==0)ppa[2*nt+1]=atof(l1);
      if(l1-l==12&&strncmp(l,"BEGINRATINGS",l1-l)==0)break;
    }
    while(fgets(l,1000,fp)){
      l1=getsp(l);assert(l1[0]==' ');
      l1[0]=0;
      t=s2n(l,0);if(t==-1)continue;// look up team 'l' to find its team number in the current league
      assert(t<nt);
      sscanf(l1+1,"%lf %lf",&x,&y);
      ppa[t]=x+poff[cl]-poff[cl0];
      ppa[nt+t]=y+poff[cl]-poff[cl0];
      ppaiv[t]=ppaiv[nt+t]=40;
      pln[t]=cl;
      if(cl!=cl0){ppaiv[t]*=.7;ppaiv[nt+t]*=.7;}
      nf++;
    }
  }
  for(t=0,x=y=0;t<nt;t++){x+=ppa[t];y+=ppa[nt+t];}
  for(t=0;t<nt;t++){ppa[t]-=x/nt;ppa[nt+t]-=y/nt;}
  printf("Found %d out of %d teams from previous year's ratings\n",nf,nt);
  if(nf<nt&&cl0<4)printf("Warning: %d unknown team%s\n",nt-nf,nt-nf==1?"":"s");

  printf("%*s---- Prior ----\n",maxtl/2+14,"");
  printf("%-*s  pl cl    Attack      sd   Defence      sd\n",maxtl,"Team");
  for(t=0;t<nt;t++)printf("%-*s  %2d %2d   %7.3f %7.3f   %7.3f %7.3f\n",
			  maxtl,tm[t],pln[t],cl0,ppa[t],1/sqrt(ppaiv[t]),ppa[nt+t],1/sqrt(ppaiv[nt+t]));
  for(i=0;i<2;i++)printf("%7.3f %7.3f\n",ppa[2*nt+i],1/sqrt(ppaiv[2*nt+i]));
  printf("\n");

  for(t=0;t<2*nt+2;t++)ppaiv[t]*=1;
}

int cmpr(const void*x,const void*y){
  double z;
  z=relp[*((int*)y)]-relp[*((int*)x)];
  return (z>0)-(z<0);
}
int main(int ac,char **av){
  int a,b,c,d,i,j,k,n,r,t,cl,m0,m1,s0,s1,ts,no,nr0,ss[MAXS][MAXS],rs[MAXS],cs[MAXS],ord[MAXNT];
  double x,y,ex,ob,la,mu,chi,der;
  char l[1000],*l2,*l3,l4[100],now0[1000],now1[1000],lmdt[100];
  time_t t0;
  FILE *fp,*fpi,*fpo;
  initargs(ac,av);
  t0=time(0);assert(gmtime_r(&t0,&now));
  strftime(now0,1000,"%Y%m%d-%H%M%S",&now);
  strftime(now1,1000,"%A %e %B %Y at %T GMT",&now);
  strftime(l,1000,"%Y-%m-%dT%H:%M:%S",&now);if(strcmp(l,cutofftime)<0)strcpy(cutofftime,l);
  printf("Analysis at %s\n",now1);
  loadequivnames();
  for(i=1,lfac[0]=0;i<MAXS;i++)lfac[i]=lfac[i-1]+log(i);
  for(i=0;adjall[i].y&&adjall[i].y!=year;i++);
  aa=&adjall[i];if(aa->y==0)printf("Warning - no adjustments found for year %d\n",year);
  for(cl=0;cl<5;cl++){
    if(ln>=0&&cl!=ln)continue;
    sprintf(lmdt,"%sT23:59:59",aa->mdt[cl]);aa->mdt[cl]=lmdt;
    if(strcmp(cutofftime,aa->mdt[cl])<0)aa->mdt[cl]=cutofftime;
    acc=rej=0;
    maxtl=10;// maxtl>=10 to line up "AWAYDISADV"
    getname(year,cl,divname);
    sprintf(datadir,"data/%02d%02d/div%d",year%100,(year+1)%100,cl);
    sprintf(l,"mkdir -p %s",datadir);assert(system(l)==0);
    last=-1;
    if(update){// fetch results
      sprintf(l,"/usr/bin/python getresults.py %d %d",cl,year);fpi=popen(l,"r");
      sprintf(l,"%s/results",datadir);fpo=fopen(l,"w");assert(fpo);
      while(fgets(l,1000,fpi))fprintf(fpo,"%s",l);
      pclose(fpi);
      if(aa->r)for(i=0;aa->r[i].t0;i++)if(aa->r[i].d==cl){// Adjustment results (extra matches)
        fprintf(fpo,"%s %s %s %d %d",aa->r[i].dt,aa->r[i].t0,aa->r[i].t1,aa->r[i].s0,aa->r[i].s1);
      }
      fclose(fpo);
    }
    nt=0;nr=0;no=0;
    sprintf(l,"%s/teams",datadir);fp=fopen(l,"r");
    if(fp){
      while(fgets(l,1000,fp)){l[strlen(l)-1]=0;s2n(l,1);}
      fclose(fp);
    }
    sprintf(l,"%s/results",datadir);fp=fopen(l,"r");assert(fp);
    while(fgets(l,1000,fp)){
      assert(nr<MAXNM);
      l2=l;l3=getsp(l2);*l3=0;
      strcpy(l4,l2);if(strlen(l4)==10)strcat(l4,"T14:00:00");// Give matches with unknown times a nominal 2pm kickoff
      if(strcmp(l4,aa->mdt[cl])>0)continue;
      res[nr][4]=secs(l2);
      l2=nonsp(l3+1);l3=getsp(l2);*l3=0;res[nr][0]=s2n(l2,1);
      l2=nonsp(l3+1);l3=getsp(l2);*l3=0;res[nr][1]=s2n(l2,1);
      n=sscanf(l3+1,"%d %d %lf %lf %lf",&res[nr][2],&res[nr][3],&odds[nr][0],&odds[nr][1],&odds[nr][2]);
      assert(n==2||n==5);
      no+=(n==5);
      if(last==-1||res[nr][4]>res[last][4])last=nr;// record the last match played
      nr++;
    }
    fclose(fp);
    assert(no==0||no==nr);
    //printf("no=%d\n",no);
    getpriorparams(cl);
    qsort(res,nr,5*sizeof(int),cmpres);
    checkres();
    nr0=nr;
    nr=MIN(nr,useres);
    printf("%d team%s\n",nt,nt==1?"":"s");
    printf("%d result%s, of which using %d\n",nr0,nr0==1?"":"s",nr);
    //if(nt==0)return 0;

    if(rel){
      int nt0;
      sprintf(l,"%s/table",datadir);fp=fopen(l,"r");
      if(!fp){fprintf(stderr,"Can't use rel>0 option without running through once normally first\n");exit(1);}
      do assert(fgets(l,1000,fp)); while(strncmp(l,"Team",4));
      nt0=nt;n=0;
      for(i=0;i<MAXP;i++){
	j=prop[cl][i];
	if(j==0){fprintf(stderr,"Couldn't find relegation property in division %d\n",cl);exit(1);}
	if(j<-200&&j>-300)break;
      }
      while(fgets(l,1000,fp))if(l[0]!=' '){
        l2=getsp(l);l2[0]=0;t=s2n(l,1);l2[0]=' ';n++;
	if(nt>nt0){fprintf(stderr,"Team '%s' in old table is not recognised\n",tm[nt-1]);exit(1);}
	rell[t]=t;relp[t]=atof(col(4+i,l));
      }
      if(n!=nt){fprintf(stderr,"There are some missing teams in old table\n");exit(1);}
      qsort(rell,nt,sizeof(int),cmpr);
      //for(i=0;i<nt;i++)printf("%s %g\n",tm[rell[i]],relp[rell[i]]);
      for(i=0;i<nt;i++)relm[i]=-1;
      for(i=0;i<rel;i++)relm[rell[i]]=i;
      printf("Relegation set: ");
      for(i=0;i<rel;i++)printf("%s ",tm[rell[i]]);
      printf("\n");
      fclose(fp);
    }

    for(i=0;i<nt;i++)sc[i]=gd[i]=gs[i]=pg[i]=0;
    for(r=0;r<nr;r++){
      a=res[r][0];b=res[r][1];c=res[r][2];d=res[r][3];
      sc[a]+=(c>d?3:(c==d?1:0));
      sc[b]+=(d>c?3:(c==d?1:0));
      gd[a]+=c-d;gd[b]+=d-c;
      gs[a]+=c;gs[b]+=d;
      pg[a]++;pg[b]++;
    }
    for(i=0;i<nt;i++){scu[i]=sc[i];gdu[i]=gd[i];}// Unadjusted copy
    for(i=0;aa->s[i].t;i++)if(aa->s[i].d==cl)for(j=0;j<nt;j++)if(strcmp(aa->s[i].t,tm[j])==0)sc[j]+=aa->s[i].a;
    for(i=0;i<MAXS;i++)for(j=0;j<MAXS;j++)ss[i][j]=0;
    for(i=0,m0=m1=0;i<nr;i++){
      s0=res[i][2];s1=res[i][3];assert(s0<MAXS&&s1<MAXS);
      if(s0>m0)m0=s0;
      if(s1>m1)m1=s1;
      ss[s0][s1]++;
    }
    for(i=0;i<=m0;i++){
      printf("%2d: ",i);
      for(j=0;j<=m1;j++)printf("%3d ",ss[i][j]);
      printf("\n");
    }
    //m0=m1=3;
    for(i=0;i<=m0;i++)for(j=0,rs[i]=0;j<=m1;j++)rs[i]+=ss[i][j];
    for(j=0;j<=m1;j++)for(i=0,cs[j]=0;i<=m0;i++)cs[j]+=ss[i][j];
    for(i=0,t=0;i<=m0;i++)t+=rs[i];ts=t;
    for(j=0,t=0;j<=m1;j++)t+=cs[j];assert(ts==t);
    chi=0;der=0;
    for(i=0;i<=m0;i++)for(j=0;j<=m1;j++){
      ex=rs[i]*cs[j]/(double)ts;
      ob=ss[i][j];
      x=fabs(ob-ex);
      chi+=x*x/ex;
      der+=ob/ex-1;
    }
    printf("chi^2 %g\n",chi);
    printf("DOF %d\n",m1*m0);
    printf("der %g\n",der);
    for(i=0;i<=m0;i++)phg[i]=rs[i]/(double)ts;
    for(j=0;j<=m1;j++)pag[j]=cs[j]/(double)ts;
    printf("Home: ");for(i=0;i<=m0;i++)printf("%10g ",phg[i]);printf("\n");
    printf("Away: ");for(j=0;j<=m1;j++)printf("%10g ",pag[j]);printf("\n");
    for(i=0,x=0;i<=m0;i++)x-=phg[i]?phg[i]*log(phg[i]):0;
    for(j=0,y=0;j<=m1;j++)y-=pag[j]?pag[j]*log(pag[j]):0;
    printf("Simple bits/match (home score): %g\n",x/log(2));
    printf("Simple bits/match (away score): %g\n",y/log(2));
    printf("Simple bits/match (both): %g\n",(x+y)/log(2));

    np=2*nt+2;
    al=pa;be=pa+nt;hh=pa+2*nt;
    
    if(eval){doeval();continue;}

    opt();
    for(i=0;i<nt;i++)ord[i]=i;
    qsort(ord,nt,sizeof(int),cmps);
    for(k=0;k<1+(nr==nt*(nt-1));k++){
      if(k==0)sprintf(l,"%s/ratings",datadir); else sprintf(l,"%s/finalratings",datadir);
      fp=fopen(l,"w");assert(fp);
      fprintf(fp,"%-*s  %12g\n%-*s  %12g\n",maxtl,"HOMEADV",hh[0],maxtl,"AWAYDISADV",hh[1]);
      fprintf(fp,"BEGINRATINGS\n");
      for(i=0;i<nt;i++){t=ord[i];fprintf(fp,"%-*s  %12g  %12g\n",maxtl,tm[t],al[t],be[t]);}
      fclose(fp);
    }
    for(r=0,x=y=0;r<nr;r++){
      a=res[r][0];b=res[r][1];c=res[r][2];d=res[r][3];
      la=fn(al[a]-be[b]+hh[0]);
      mu=fn(al[b]-be[a]-hh[1]);
      x+=poisent(la);
      y+=poisent(mu);
    }
    printf("Self bits/match (home score): %g\n",x/nr/log(2));
    printf("Self bits/match (away score): %g\n",y/nr/log(2));
    printf("Self bits/match (both): %g\n",(x+y)/nr/log(2));

    sim(cl);

  }//cl

  if(dohtml){sprintf(l,"/usr/bin/python dohtml.py %d",year);assert(system(l)==0);}
  return 0;
}
