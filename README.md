# WebServer
My first Cpp project: a HTTP Web Server. **Proacter** simulated with **Synchronize IO**, and applied **IO Multiplexing** using **epoll API**. I first meant to build a web server from scratch, but to do my homework more quickly, I end up using [Apache2](https://ubuntu.com/tutorials/install-and-configure-apache#1-overview) and [cgicc](https://www.gnu.org/software/cgicc/index.html). I also used [webbench](https://github.com/tamlok/webbench.git) to do preasure test. I was having a [course](https://www.nowcoder.com/study/live/504) to learn about Linux, Pipe, Thread and Unix Network Programming, the Sample code was also include.
## Skills
In this project, I tried to write **Makefile** and **Doxygen** comment. I may try **CMake** after, and incorporate **MySQL**.
## Apache2
You can follower the [tutorials](https://ubuntu.com/tutorials/install-and-configure-apache#1-overview) at webset, and you can also run my script at CGI/SetUp/install_apache2.sh
```shell
sudo apt update && sudo apt upgrade
sudo apt-get install apache2
sudo /etc/init.d/apache2 restart
echo "for wsl curl must be run windows"
curl http://127.0.0.1
echo "you add your cgi-script to /usr/lib/cgi-bin/"
echo "edit /etc/apache2/conf-enabled/serve-cgi-bin.conf"
echo "AllowOverride All"
echo "AddHandler cgi-script .cgi .py .sh .pl"
echo "sudo ln -s /etc/apache2/mods-available/cgi.load /etc/apache2/mods-enable/cgi.load"

# uninstall
# sudo apt remove apache2 & sudo apt purge apache2
# you can then set if .cgi is runnable after restart
```
## cgicc
run shell script from CGI/SetUp/install_cgicc.sh
```shell
curl ftp://ftp.gnu.org/gnu/cgicc/cgicc-3.2.19.tar.gz -o cgicc-3.2.19.tar.gz
tar xzf cgicc-3.2.19.tar.gz 
cd cgicc-3.2.19/ 
./configure --prefix=/usr 
make
sudo make install
```
and it failed on OSX. 
then compile you cgi script using
```shell
g++ -o %.cgi %.cpp -lcgicc # you can in fact ignore .cgi
sudo cp *.cgi /usr/lib/cig-bin # $(destination) /usr/lib/cgi-bin for example
# don't forget to check state of your cgi-script at destination to make sure it's runnable
```
then test it
```shell
curl http://127.0.0.1/cgi-bin/$(your cgi-script)
```
## webbench
It's included in the project, you recompile it and test
```shell
make 
./webbench -h
./webbench -c 10000 -t 5 http://127.0.0.1/index.html
```
and I compile my code at github codespace, I tried vscode.dev and linked to github.dev. Give it a try.
