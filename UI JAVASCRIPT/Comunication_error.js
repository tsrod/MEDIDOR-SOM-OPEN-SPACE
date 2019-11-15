var error=0;
var errout={};
var t=0
var date = new Date();
var tatual=date.getTime();
var tant=context.get('time') || tatual;


t=tatual-tant;

if (t>10000){error=1;}
else{    error=0;}


msg.payload=t;
errout.payload=error;

context.set('time',date.getTime());
return  [msg,errout];