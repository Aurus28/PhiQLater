#!/usr/bin/bash
git pull
flatpak uninstall de.aurus28.PhiQLater -y
flatpak install --user phiqlater.flatpak -y