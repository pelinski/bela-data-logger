for REMOTE in "bela.A" "bela.B" "bela.C"; do echo $REMOTE && scp render.cpp $REMOTE:Bela/projects/bela-data-logger/; done