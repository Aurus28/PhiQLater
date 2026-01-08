# Steps to get to a .flatpak file


1. Prerequisites: Have the repository set (see 'install.sh')
2. Build the App (run in source directory):

   ```flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo build-dir flatpak/de.aurus28.PhiQLater.json```
3. bundle that into a single '.flatpak' file (also run in source directory):

   ```flatpak build-bundle repo phiqlater.flatpak de.aurus28.PhiQLater --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo```