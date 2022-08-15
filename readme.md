# bela-data-logger

This code uses [@adanlbenito](https://github.com/adanlbenito)'s BelaParallelComm and the Bela WriteFile library to send or receive clock signals across Belas and log sensor data to binary files. It can be used to record signals from multiple sensors connected to different Belas. You can define a Bela master (TX) that will send a clock signal to the Bela receivers (RXs). These clock signals are also stored in binary files and can be used afterwards to sync the sensor data from the receiver Belas to the transmitter Bela (code available here: [bela-data-syncer](https://github.com/pelinski/bela-data-syncer)).

## Instructions

- Connect the Bela master digital IO pins 0 and 1 to the Bela receivers 0 and 1 pins.
- If you are using the same host machine to ssh into multiple Belas, you will need to change their default IP addresses. Instructions to do so are in the [knowledge base](https://learn.bela.io/using-bela/technical-explainers/ip-addresses/).
- If you have cloned this repo in your host machine in some path, you can copy it to the Belas by using (from the host machine):

  ```
  scp -r path/to/where/you/cloned/this/repo root@[the.bela.ip]:Bela/projects/
  ```

  You can also do it in bulk for all the Belas at once by doing (from the host machine):

  ```
  for REMOTE in "root@[bela.master.ip]:Bela/projects/" "root@[bela.receiver1.ip]:Bela/projects/" "root@[bela.receiver2.ip]:Bela/projects/"; do scp -r path/to/where/you/cloned/the/repo/ $REMOTE; done
  ```

- Now you can compile the code for the Bela master. Ssh into the Bela master by doing `ssh root@[bela.master.ip]` and then run the following command:

  ```
  make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=1"
  ```

- You can now compile the code for the receiver Belas. Ssh into the receiver Bela by doing `ssh root@[bela.receiverN.ip]` and then run the following command (substitute N for the Bela receiver ID (0,1,2,..), each Bela receiver should have its unique ID).

  ```
  make -C /root/Bela PROJECT=bela-data-logger CPPFLAGS="-DBELA_MASTER=0" CPPFLAGS="-DBELA_ID=N"
  ```

- Now you can run the script in every Bela. It is very important that the **receiver Belas start running before the transmitter Bela**. This assures that the first clock signal received at RXN is also the first clock signal sent by TX.
  ```
  cd Bela/projects/bela-data-logger && ./bela-data-logger
  ```
- The Belas are now recording the sensor values and the clock signals. Do ctrl+C on every Bela Terminal to interrupt the communication.
- Once you are happy with the recorded signals, you can copy them into your host machine by doing (from the host machine):
  ```
  for REMOTE in "root@[bela.master.ip]:Bela/projects/bela-data-logger" "root@[bela.receiver1.ip]:Bela/projects/bela-data-logger" "root@[bela.receiver2.ip]:Bela/projects/bela-data-logger"; do scp $REMOTE\*.log path/where/you/want/the/logs/in/your/host/machine; done
  ```
- Now you can use the [bela-data-syncer](https://github.com/pelinski/bela-data-syncer) library to sync the sensor signals across Belas.
