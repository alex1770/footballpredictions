#!/usr/bin/python

import urllib2,csv,sys,datetime,time,re,bs4

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
  y=x[:-1].split(' ')
  for z in y: eq[z]=y[0]
f.close()

def cap(x):# 'AFC telford utd' -> 'AFC Telford Utd'
  l=[]
  for y in x.split(): l.append(y[0].upper()+y[1:])
  return ' '.join(l)

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

def getfromsoccernet(year,ln):
  suffix=['eng.1/barclays-premier-league',
          'eng.2/english-league-championship',
          'eng.3/english-league-one',
          'eng.4/english-league-two',
          'eng.5/english-conference'][ln]
  url='http://soccernet.espn.go.com/results/_/league/'+suffix
  # The string after eng.x/ appears to be irrelevant
  # It's possible the site could throw up a popup which messes stuff up
  u=urllib2.urlopen(url)
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[];dt=None;count=10
  for x in soup.find_all(True):
    if x.name=='tr' and x['class']=='stathead':
      y=x.td
      if y:
        y=y.text# Expecting a date like "Wednesday, January 2, 2013"
        try:
          t=time.strptime(y.strip(),'%A, %B %d, %Y')
        except ValueError:
          continue
        dt=time.strftime('%Y-%m-%d',t)
    if x.name=='tr' and (x['class'][:6]=='oddrow' or x['class'][:7]=='evenrow'):
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
  suffix=['premier-league','championship','league-one','league-two','football-conference'][ln]
  suffix+='/%d-%d/results/'%(year,year+1)
  url='http://www.scorespro.com/soccer/england/'+suffix
  u=urllib2.urlopen(url)
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[];dt=None;ok=0;playoff=None
  for x in soup.find_all(True):
    if x.name=='div' and 'class' in x.attrs and x['class']=='ncet':
      fc=1
      for y in x.children:
        z=y.text.strip()
        if fc: playoff='play off' in z.lower() or 'play-off' in z.lower()
        fc=0
        try:
          t=time.strptime(z,'%a %d %b %Y')
          dt=time.strftime('%Y-%m-%d',t)
        except ValueError:
          continue
    elif x.name=='table': ok=0
    elif x.name=='td' and 'class' in x.attrs:
      y=x['class']
      if y=='status':
        for z in x.children:
          if z.text=='FT': ok=1;hteam=ateam=hgoals=agoals=None
      if playoff or not ok: continue
      if y[:4]=='home': hteam=cap(x.text.strip())
      elif y[:4]=='away':
        ateam=cap(x.text.strip())
        if dt and hteam and ateam and hgoals!=None and agoals!=None:
          l.append((dt,std(hteam),std(ateam),hgoals,agoals))
      elif y=='score':
        z=x.text;f=z.find('-')
        if f>=0: hgoals=int(z[:f]);agoals=int(z[f+1:])
  return l

def getfrombbc(year,ln):
  suffix=['118996114','118996115','118996116','118996117','118996118'][ln]
  url='http://www.bbc.co.uk/sport/football/results?filter=competition-'+suffix
  u=urllib2.urlopen(url)
  soup=bs4.BeautifulSoup(u,"html5lib")
  l=[];dt=None;playoff=None
  for x in soup.find_all(True):
    if 'class' not in x.attrs: continue
    cl=x['class']
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
        if x[0]<maxd: err("Error: %s"%n+" omission "+str(d[x]))
        else: err("Warning: %s"%n+" slow update "+str(d[x]))
    for x in e:
      if x not in d: err("Error: %-20s"%n+"Spurious "+str(e[x]))
    for x in e:
      if x in d and d[x]!=e[x]: err("Error: %s"%n+" wrong "+str(e[x])+" cf GTR "+str(d[x]))

# Third parameter is whether the site in question provides past years data
# Arrange in increasing reliability (order not currently used)
parsers=[("soccernet",getfromsoccernet,0),# Quite error prone in 2012; can't distinguish play-offs from league games
         ("BBC",getfrombbc,0),# Occasional errors in years < 2012
         ("footballdata",getfromfootballdata,1),# Occasional errors in years < 2012; one error in 2012
         ("scorespro",getfromscorespro,0)]# No known errors, though not tried much
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
