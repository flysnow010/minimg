# minimg
Minimg is a http server of image.

## build
$cd minimg  
$source setenv.sh  
$jam  

## run
$minimg images/ 8080  

## test
curl http://127.0.0.1:8080/test.jpg?size=100x100 -O test.jpg   
