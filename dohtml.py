#!/usr/bin/python
import sys,datetime,cgi,os
from math import floor
from subprocess import Popen,PIPE

now=datetime.datetime.now()
if len(sys.argv)>1: year=int(sys.argv[1])
else: year=now.year-(now.month<7)

tables=[];ratings=[]
updated=""
for div in range(5):
  datadir="data/%02d%02d/div%d"%(year%100,(year+1)%100,div)
  f=open(datadir+"/table","r");table=f.read().split('\n')[:-1];f.close()
  tables.append(table)
  for x in table:
    y=x.split()
    if y[0]=='UPDATED':
      if y[1]>updated: updated=y[1]
      break
  f=open(datadir+"/ratings","r");temp=f.read().split('\n')[:-1];f.close()
  for i in range(len(temp)):
    if temp[i]=='BEGINRATINGS': break
  ratings.append(temp[i+1:])
assert updated[-5:]=="+0000"
updated=datetime.datetime.strptime(updated[:-5],'%Y-%m-%dT%H:%M:%S')

def getname(year,div):
  if year<2004: return ["Premiership","Division 1","Division 2","Division 3","Conference"][div]
  if year<2007: return ["Premiership","Championship","League 1","League 2","Conference"][div]
  if year<2015: return ["Premier League","Championship","League 1","League 2","Conference"][div]
  return ["Premier League","Championship","League 1","League 2","National League"][div]

def bbctablename(div):
  s="http://www.bbc.co.uk/sport/football/"
  s+=['premier-league','championship','league-one','league-two','national-league'][div]
  s+="/table"
  return s

up0=updated.strftime("%Y%m%d-%H%M%S")
up1=updated.strftime("%A %e %B %Y at %T GMT")
f=open("predictions/pred%s.html"%up0,"w")

print >>f,'<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">'
print >>f,"<html><head><title>Football league predictions</title></head><body>"
print >>f,"<h2><p>Prediction of final league standings, promotion and relegation</p></h2>"
print >>f,"<h3><p>Updated %s</p></h3>\n"%up1

for div in range(5):
  table=tables[div]
  divname=getname(year,div)
  i=0
  while 1:
    x=table[i];i+=1
    if x[:10]=='BEGINTABLE': break
    if x[:10]=='LASTPLAYED': last=x.split()[1:]
  f.write("<b>%s</b>&nbsp;&nbsp;&nbsp; ("%divname)
  if last[0][:1].isdigit():
    d=datetime.datetime.strptime(last[0],'%Y-%m-%d')
    print >>f,cgi.escape("Latest match accounted for: %s, %s %s %s %s;"%(d.strftime('%A %e %B %Y'),last[1],last[3],last[4],last[2])),
  print >>f,"<a href=\""+bbctablename(div)+"\">Check up-to-date</a>)"
  print >>f,"\n<pre>"
  for (n,x) in enumerate(table[i:]):
    if n==2: print >>f,""
    print >>f,cgi.escape(x)
  rname="ratings%s-%d.png"%(up0,div)
  print >>f,'<img src="%s">\n'%rname
  print >>f,"\n</pre>\n"
  amin=dmin=100;amax=dmax=-100
  for x in ratings[div]:
    y=x.split();a=float(y[1]);d=float(y[2])
    amin=min(a,amin);amax=max(a,amax)
    dmin=min(d,dmin);dmax=max(d,dmax)
  tic=0.1
  amin-=tic/2;dmin-=tic/2;amax+=tic/2;dmax+=tic/2
  s="gnuplot -e '"
  s+="set terminal png large size %d,%d;"%(((amax-amin)/tic+2)*100,((dmax-dmin)/tic+2)*100)
  s+='set output "predictions/%s";'%rname
  s+="set key off;"
  s+='set xlabel "Attacking capability";set ylabel "Defensive capability";'
  s+="set xrange [%f:%f];set yrange [%f:%f];"%(amin,amax,dmin,dmax)
  s+="set xtics %f;set ytics %f;"%(tic,tic)
  s+='plot "-" using ($2):($3):1 with labels\''
  p=Popen(s,shell=True,stdin=PIPE,close_fds=True)
  for x in ratings[div]: y=x.split();print >>p.stdin,'"'+y[0]+'"',y[1],y[2]
  p.stdin.close()

print >>f,"</html>"
f.close()

try: os.unlink('predictions/latest.html')
except OSError,(a,b):
  if a!=2: raise
os.symlink('pred%s.html'%up0,'predictions/latest.html')
f=open('predictions/index1.html','r');i1=f.read();f.close()
f=open('predictions/index1.html','w')
print >>f,"<li><a href=\"pred%s.html\">%s</a>"%(up0,up1)
f.write(i1);f.close()
f=open('predictions/index0.html','r');i0=f.read();f.close()
f=open('predictions/index1.html','r');i1=f.read();f.close()
f=open('predictions/index.html','w');f.write(i0);f.write(i1);f.close()
