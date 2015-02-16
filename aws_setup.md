Steps for reproducing the `HPCE-2014-GPU-Image` AMI.

Start with the image `debian-jessie-amd64-hvm-2014-12-25-13-19-ebs (ami-aae18fc2)`
on a GPU instance.

Then do:
```
sudo sed -i -e "s/main$/main contrib non-free/g" /etc/apt/sources.list

sudo apt-get -y update

sudo apt-get -y install nvidia-libopencl1

sudo apt-get -y install nvidia-opencl-icd

sudo apt-get -y install nvidia-opencl-dev

sudo apt-get -y install nvidia-driver

sudo reboot
```
@schmethan suggests also installing `eog` and `libcanberra-gtk3-module` for viewing images.
