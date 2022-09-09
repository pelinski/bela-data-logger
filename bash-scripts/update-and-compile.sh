for REMOTE in "bela.A" "bela.B" "bela.C"; do echo $REMOTE && scp -r ../BelaParallelComm/ ../render.cpp  $REMOTE:Bela/projects/bela-data-logger/; done
echo "--bela.A--" && ssh bela.A 'make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=1 -DBELA_ID=0"'
echo "--bela.B--" && ssh bela.B 'make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=0 -DBELA_ID=1"'
echo "--bela.C--" && ssh bela.C 'make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=0 -DBELA_ID=2"'