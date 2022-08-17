
echo "bela.A" && ssh bela.A 'make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=1 -DDIGITAL_PINS={1,2} -DNUM_ANALOG_PINS=8"'

echo "bela.B" && ssh bela.B 'make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=0 -DBELA_ID=1 -DDIGITAL_PINS={1,2} -DNUM_ANALOG_PINS=8"'

echo "bela.C" && ssh bela.C 'make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=0 -DBELA_ID=2 -DDIGITAL_PINS={1,2} -DNUM_ANALOG_PINS=4"'