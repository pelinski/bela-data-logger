These are scripts I used to deal with multiple Belas connected to the same host machine. After assigning each Bela a different I (instructions [here](https://learn.bela.io/using-bela/technical-explainers/ip-addresses/)), you can add them to your ssh config (`~/.ssh/config`):

```
Host bela.A
    HostName 192.168.8.2
    User root
Host bela.B
    HostName 192.168.9.2
    User root
Host bela.C
    HostName 192.168.5.2
    User root
```

The bash scripts in this folder use this aliases to do bulk operations on 3 Belas from the same host machine.
