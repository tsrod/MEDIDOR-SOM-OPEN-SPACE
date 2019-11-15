var auth ;
cred=msg.payload;
proj=cred[0].title;
pass=cred[1].password;

if ((proj == '<b>MEDIDOR SOM OPEN SPACE</b>') && pass == ("123")) { 
auth=1;
} else if ((proj == '<b>LAY2FORM</b>') && pass == ("123")) {
    auth=1;
} else if ((proj == '<b>PROJETO X</b>') && pass == ("123")) {
    auth=1;
} else if ((proj == '<b>PROJETO Y</b>') && pass == ("123")) {
    auth=1;
} else if ((proj == '<b>PROJETO Z</b>') && pass == ("123")) {
    auth=1;
}else
{auth=0
}

msg.payload= auth;
return msg;