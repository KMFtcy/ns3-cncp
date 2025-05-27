# NS#-CNCP

This is a coding-based non-conservative communication protocol flow control implement by ns3.

This repository inherits from previous works of [Alibaba](https://github.com/alibaba-edu/High-Precision-Congestion-Control) and [Microsoft](https://github.com/bobzhuyb/ns3-rdma) which has implemented DCTCP, DCQCN, TIMELY, HPCC congestion control.

The earlier efforts was based on ns3 version 3.17, which requires outdated tools such as Python 2 and compile tools requires glibc 4.8. A direct and serious issue is that operating systems supporting glibc 4.8 are no longer compatible with VSCode Remote development.

Hence, I moved it to the ns3-3.44 version, which is the latest one in 2025.05.

## Run

```
./ns3 run scratch/third -- mix/config.txt
```
