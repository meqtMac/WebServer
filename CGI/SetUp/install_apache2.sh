sudo apt update && sudo apt upgrade
sudo apt-get install autoconf
sudo apt-get install apache2
echo "/cgi-bin 位置 /usr/lib/cgi-bin/"
echo "AllowOverride All"
echo "AddHandler cgi-script .cgi .py .sh .pl"
echo "sudo vim /etc/apache2/conf-enabled/serve-cgi-bin.conf"
echo "sudo ln -s /etc/apache2/mods-available/cgi.load /etc/apache2/mods-enable/cgi.load"
sudo /etc/init.d/apache2 restart
echo "wsl 注意curl等操作要在windows cmd中进行"
curl http://127.0.0.1

# uninstall
# sudo apt remove apache2 & sudo apt purge apache2
