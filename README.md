**Connect digital pins {0,1,2,3} from Bela master to bela slaves**

**Connecting multiple belas to the same laptop**
They need to have different ip addresses [knowledge base](https://learn.bela.io/using-bela/technical-explainers/ip-addresses/)
master bela: `192.168.7.2` (default address) (`ssh root@192.168.7.2`)

```
ssh root@192.168.7.2
```

slave bela: `192.168.8.2` (`ssh root@192.168.8.2`)

```
ssh root@192.168.8.2
```

**compile and run for master**

```
 make -C /root/Bela PROJECT=bela_parallel_comm CPPFLAGS="-DBELA_MASTER=1" run
```

**compile and run for slave**

```
 make -C /root/Bela PROJECT=bela_parallel_comm run
```

**run**

```
./bela_parallel_comm
```

**the receiver needs to always be started before the transmitter**
