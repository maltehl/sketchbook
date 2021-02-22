Grove UV sensor VEML6070
==============

Introduction of sensor
----------------------------  
VEML6070 is an advanced ultraviolet (UV) light sensor with I2C protocol interface and designed by the CMOS process.  
It is easily operated via a simple I2C command.  
EML6070 incorporates a photodiode, amplifiers, and analog / digital circuits into a single chip. 


***
Usage:
==========
Download all the source files.
>* **exampleS/basic_demo/basic_demo.ino**,This example can get the UV index and UV level from the serial.polling for data.
>* **examples/INT_mode/INT_mode.ino**,There is a INT pad on the sensor module which connect to ACK pin of VEML6070. You can set UV threshold by 102 steps or 145 steps(only two choises).The INT pin outputs low when the UV value beyond limit.You can attach INT pin to a interrupt pin of host,To improve the efficiency of program operation.


Reference:
===============
Refer to the **/doc** to get more detail.


***
This software is written by downey  for seeed studio<br>
Email:dao.huang@seeed.cc
and is licensed under [The MIT License](http://opensource.org/licenses/mit-license.php). Check License.txt for more information.<br>

Contributing to this software is warmly welcomed. You can do this basically by<br>
[forking](https://help.github.com/articles/fork-a-repo), committing modifications and then [pulling requests](https://help.github.com/articles/using-pull-requests) (follow the links above<br>
for operating guide). Adding change log and your contact into file header is encouraged.<br>
Thanks for your contribution.

Seeed Studio is an open hardware facilitation company based in Shenzhen, China. <br>
Benefiting from local manufacture power and convenient global logistic system, <br>
we integrate resources to serve new era of innovation. Seeed also works with <br>
global distributors and partners to push open hardware movement.<br>
