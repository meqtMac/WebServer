curl ftp://ftp.gnu.org/gnu/cgicc/cgicc-3.2.19.tar.gz -o cgicc-3.2.19.tar.gz
tar xzf cgicc-3.2.19.tar.gz 
cd cgicc-3.2.19/ 
./configure --prefix=/usr 
make
sudo make install
