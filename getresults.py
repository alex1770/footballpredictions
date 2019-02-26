#!/usr/bin/python

import urllib2,csv,sys,datetime,time,re,bs4,string
from subprocess import PIPE,Popen

def err(s):
  print >>sys.stderr,datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'),s

err("Info: starting getresults.py "+' '.join(sys.argv[1:]))

if len(sys.argv)<=1:
  print >>sys.stderr,"Usage:",sys.argv[0],"<league> [year]"
  print >>sys.stderr,"       league = 0, 1, 2, 3 or 4"
  sys.exit(1)

ln=int(sys.argv[1])# league number
t=datetime.datetime.now();now=t.year-(t.month<7)
# Seasons are specified by the starting year
if len(sys.argv)>2: year=int(sys.argv[2])
else: year=now

f=open('equivnames','r')
eq={}
for x in f:
  if x[:1]=='#': continue
  y=x.strip().split(' ')
  for z in y: eq[z]=y[0]
f.close()

def cap(x):# 'AFC telford utd' -> 'AFC Telford Utd'
  l=[]
  for y in x.split(): l.append(y[0].upper()+y[1:])
  return ' '.join(l)

def getnums(x):
  x=[(y if y.isdigit() else ' ') for y in x]
  x=''.join(x)
  return map(int,x.split())

def text(x):
  ok=string.lowercase+string.uppercase+"'-& "
  return ''.join(y for y in x if y in ok)

ordn=re.compile(r'(?<=\d)(st|nd|rd|th)\b')
def deord(x): return ordn.sub('',x)# Remove ordinal suffixes

def std(x): return eq[x.strip().replace(' ','_')]# Convert a team name into 'standard form'

# Parsers should return a list containing tuples like
# ('2012-09-15', 'Arsenal', 'Southampton', 6, 1)
#  Date, Hometeam, Awayteam, Homegoals, Awaygoals
# Ideally they should fail silently: errors will be picked up by the comparison stage

# Uses http://www.football-data.co.uk/
# Very nice and clean csv and has many past years, but can be a few days out of date
def getfromfootballdata(year,ln):
  league=['E0','E1','E2','E3','EC'][ln]
  u=urllib2.urlopen('http://www.football-data.co.uk/mmz4281/%02d%02d/'%
                    (year%100,(year+1)%100)+league+'.csv')
  re=csv.reader(u)
  l=[]
  for r in re:
    if r[0]==league:
      dt=r[1]
      yr=['19','20'][dt[-2:]<'50']+dt[-2:]
      mon=dt[3:5]
      day=dt[:2]
      if r[4]!='' and r[5]!='':# Future fixtures can appear as entries with empty scorelines
        l.append((yr+'-'+mon+'-'+day,std(r[2]),std(r[3]),int(r[4]),int(r[5])))
  return l

# This parser currently kaput since they changed everything for 2014-15
def getfromsoccernet(year,ln):
  suffix=['barclays-premier-league/23/scores',
          'english-league-championship/24/scores',
          'english-league-one/25/scores',
          'english-league-two/26/scores',
          'english-conference/27/scores'][ln]
  url='http://www.espnfc.com/'+suffix
  # The string after eng.x/ appears to be irrelevant
  # It's possible the site could throw up a popup which messes stuff up
  u=urllib2.urlopen(url)
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[];dt=None;count=10
  for x in soup.find_all(True):
    if x.name=='tr' and 'stathead' in x['class']:
      y=x.td
      if y:
        y=y.text# Expecting a date like "Wednesday, January 2, 2013"
        try:
          t=time.strptime(y.strip(),'%A, %B %d, %Y')
        except ValueError:
          continue
        dt=time.strftime('%Y-%m-%d',t)
    if x.name=='tr' and (x['class'][0][:6]=='oddrow' or x['class'][0][:7]=='evenrow'):
      for y in x.find_all('td'):
        z=y.text
        if z=='FT': count=0;hteam=ateam=hgoals=agoals=None
        elif count==1: hteam=z
        elif count==2: 
          f=z.find('-')
          if f>=0: hgoals=int(z[:f]);agoals=int(z[f+1:])
        elif count==3:
          ateam=z
          if dt and hteam and ateam and hgoals!=None and agoals!=None:
            l.append((dt,std(hteam),std(ateam),hgoals,agoals))
        count+=1
  return l

def getfromscorespro(year,ln):
  suffix=['premier-league','championship','league-one','league-two','national-league'][ln]
  name=suffix.replace('-',' ')
  suffix+='/%d-%d/results/'%(year,year+1)
  url='http://www.scorespro.com/soccer/england/'+suffix
  u=urllib2.urlopen(url)
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[]
  for x in soup.find_all('tr'):
    m=[None]*5
    for y in x.find_all('td'):
      if 'class' in y.attrs:
        c=y['class']
        if 'kick_t' in c:
          for z in y.find_all('span',attrs={'class':'kick_t_dt'}):
            (dd,mm,yy)=map(int,z.text.split('.'))
            m[0]="%4d-%02d-%02d"%(yy+1900+100*(yy<60),mm,dd)
        if 'home' in c: m[1]=std(text(y.text))
        if 'away' in c: m[2]=std(text(y.text))
        if 'score' in c:
          sc=getnums(y.text)
          if len(sc)>=2: m[3]=sc[0];m[4]=sc[1]
    if all(z!=None for z in m): l.append(tuple(m))
  return l

def getfromflashscores(year,ln):
  suffix=['premier-league','championship','league-one','league-two','national-league'][ln]
  name=suffix.replace('-',' ')
  suffix+='/results/'
  url='http://www.flashscores.co.uk/football/england/'+suffix
  p=Popen('google-chrome --headless --dump-dom '+url,shell=True,close_fds=True,stdout=PIPE)
  u=p.stdout.read()
  p.stdout.close()
  if p.wait()!=0: raise Exception("Error with google-chrome subprocess")
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[]
  for x in soup.find_all('tr'):
    m=[None]*5
    for y in x.find_all('td'):
      if 'class' in y.attrs:
        c=y['class']
        if 'time' in c:
          tt=getnums(y.text)
          if len(tt)>=2: dd=tt[0];mm=tt[1];m[0]="%4d-%02d-%02d"%(year+(mm<7),mm,dd)
        if 'team-home' in c: m[1]=std(text(y.text))# Filter out &nbsp;s used for red cards
        if 'team-away' in c: m[2]=std(text(y.text))#
        if 'score' in c:
          sc=getnums(y.text)
          if len(sc)>=2: m[3]=sc[0];m[4]=sc[1]
    if all(z!=None for z in m): l.append(tuple(m))
  return l


def getfrombbc(year,ln):
  suffix=['118996114','118996115','118996116','118996117','118996118'][ln]
  url='http://www.bbc.co.uk/sport/football/results?filter=competition-'+suffix
  u=urllib2.urlopen(url)
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[];dt=None;playoff=None
  for x in soup.find_all(True):
    if 'class' not in x.attrs: continue
    cl=x['class'][0]
    if cl=='table-header':
      z=x.text.strip()
      try:
        t=time.strptime(deord(z),'%A %d %B %Y')
        dt=time.strftime('%Y-%m-%d',t)
      except ValueError:
        continue
    if cl=='competition-title':
      z=x.text.lower()
      playoff='play-off' in z or 'play off' in z
    if playoff: continue
    if x.name=='td' and cl=='time' and x.text.strip()=='Full time':
      if dt and hteam and ateam and hgoals!=None and agoals!=None:
        l.append((dt,std(hteam),std(ateam),hgoals,agoals))
    if x.name!='span': continue
    if cl[:9]=='team-home': hteam=x.text.strip();ateam=hgoals=agoals=None
    elif cl=='score':
      z=x.text.strip();f=z.find('-')
      if f>=0:
        try: hgoals=int(z[:f]);agoals=int(z[f+1:])
        except: pass
    elif cl[:9]=='team-away': ateam=x.text.strip()
  return l

def oldmerge(l1,l2):
  d={}
  for x in l1+l2:
    k=x[1:3]
    if k not in d: d[k]=x
    elif d[k]!=x:
      err("Warning: incompatible results (taking last to be correct)")
      err("Warning: %s  %-19s %-19s %2d %2d\n%s  %-19s %-19s %2d %2d\n"%(d[k]+x))
      d[k]=x
  kk=list(d);kk.sort()
  return [d[x] for x in kk]

def getgroundtruth(pp):
  # Could have slightly better error checking if assumed that each home-away pairing can
  # only occur once, but it's convenient to index by date-home-away.  This has the merit
  # of working for leagues (e.g., SPL) where home-away pairings can occur multiple times
  # per season.
  d={};mind={};maxd={}
  for n in pp:
    for x in pp[n]:
      d.setdefault(x[:3],[]).append((n,x))
      if n not in mind or x[0]<mind[n]: mind[n]=x[0]
      if n not in maxd or x[0]>maxd[n]: maxd[n]=x[0]
  #for x in d: print d[x]
  #for n in mind: print n,mind[n],maxd[n]
  l=[]
  for x in d:
    cl=d[x];e={}
    for (n,y) in cl: e[n]=1
    av=0
    for n in mind:
      if n not in e and mind[n]<x[0] and x[0]<maxd[n]: av+=1# Count antivotes
    #if av>0: print cl
    # Decompose cl into equiv classes and accept largest class if size(class)>=av
    # If equally large then prioritise some feeds over others (todo)
    f={}
    for x in cl: f.setdefault(x[1],[]).append(x[0])
    m=0;tr=None
    for x in f:
      if len(f[x])>m: m=len(f[x]);tr=x
    if tr and m>=av: l.append(tr)
    #print cl;print f;print tr;print m;print
  l.sort()
  return l

def check(pp,gtr):
  d={}
  for x in gtr: d[x[:3]]=x
  for n in pp:
    mind='9999-12-31';maxd='0000-01-01'
    for x in pp[n]: mind=min(mind,x[0]);maxd=max(maxd,x[0])
    e={}
    for x in pp[n]: e[x[:3]]=x
    for x in d:
      if x not in e and x[0]>mind:
      # The x[0]>mind condition doesn't count omissions that occurred outside the
      # date range of returned results, because some results are "rolling" and
      # only intend to give the last month, say.
        if x[0]<maxd: err("Error: %s"%n+" omission "+str(d[x]))
        else: err("Warning: %s"%n+" slow update "+str(d[x]))
    for x in e:
      if x not in d: err("Error: %-20s"%n+"Spurious "+str(e[x]))
    for x in e:
      if x in d and d[x]!=e[x]: err("Error: %s"%n+" wrong "+str(e[x])+" cf GTR "+str(d[x]))

# Third parameter is whether the site in question provides past years data
# Arrange in increasing reliability (order not currently used)
parsers=[
  #("soccernet",getfromsoccernet,0),# Quite error prone in 2012; can't distinguish play-offs from league games; broken in 2014-15 - gone all fancy schmancy
  #("BBC",getfrombbc,0),# Occasional errors in years < 2012; broken since 2017?
  ("footballdata",getfromfootballdata,1),# Occasional errors in years < 2012; one error in 2012, one minor (date) error in 2018
  ("scorespro",getfromscorespro,0),# One problem in 2018-19 due to misnaming Mansfield Town
  ("flashscores",getfromflashscores,0)]# Introduced 2019-02-25
pp={}
for (n,g,p) in parsers:
  if year==now or p:
    try:
      pp[n]=g(year,ln)
      err("Info: parser %s returned %d results"%(n,len(pp[n])))
    except Exception as x:
      err("Error: parser %s failed with exception %s: %s"%(n,type(x).__name__,str(x)))
gtr=getgroundtruth(pp)
for x in gtr: print "%s  %-19s %-19s %2d %2d"%x
check(pp,gtr)
