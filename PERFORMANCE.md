# Performance test

This test shows the limits of the library in terms of performace in case of large data transfer/streaming of data. The test settings are based on typical application Arduino(MCU) <-> Pi communication. A low-end RPi is used to have a conservative baseline in terms of transfer rates & cpu usage. The porformance on more powerfull boards could improve the performance considerably.

On the other hand the benchmark reference of typical transfer rates using optimized library in micro controllers is:

| Speed  | w/ ACK    | w/o ACK |
| -----  | --------- | ------- |
| 2Mbps  | 68KB/s    | 200KB/s |
| 1Mbps  | 47KB/s    | 100KB/s |
| 250Kbps| 16KB/s    | 22KB/s |


## Testing settings

1. Node A: Arduino Nano 5v/16Mhz
2. Node B: Rapsberry Pi Zero W 1.1 (Forced to 700Mhz) with SPIDEV driver.
3. Node A <-> Node B distance: in the same room about 2-3 meters: No packet loss.
4. Channel: 70
5. CRC: 8 bits
6. IRQ connceted (not using IRQ will reduce the transfer rate big time)
7. Max steam size 512 bytes.


## Results

### Transfer A -> B

| Speed  | w/ ACK    | w/o ACK | %CPU node B |
| -----  | --------- | ------- | ----------- |
| 2Mbps  | 62kb/s    |  loss   | 60%         |
| 1Mbps  | 33Kb/s    |  loss   | 55%         |
| 250Kbps| 11Kb/s    | 19kb/s  | 35%         |

### Transfer B -> A

| Speed  | w/ ACK    | w/o ACK | %CPU node B | % CPU node B |
| -----  | --------- | ------- | ----------- | ------------ |
| 2Mbps  |  60Kb/s   | 100kb/s |  79%        | 86%          |
| 1Mbps  |  32Kb/s   |  80Kb/s |  75%        | 82%          |
| 250Kbps|  11Kb/s   |  20Kb/s |  75%        | 80%          |

## Recomendations

- Use allways IRQ line for improve transfer speeds and reduce CPU usage.
- Streaming from a PI will stress the cpu at high rates of transfers. Using faster & multiple core boards such RPI3 is recommended for streaming.
- Tunning max stream size variable can improve the transfer speeds.
