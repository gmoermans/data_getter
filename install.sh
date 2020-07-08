#! /bin/bash
make
sudo mv data_getter.service /etc/systemd/system/
sudo cp data_getter /usr/bin/
sudo systemctl enable data_getter.service
sudo systemctl start data_getter.service

