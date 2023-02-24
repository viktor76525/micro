#!/bin/bash
# start a compressed virtualbox virtual machine
# which follows the set pattern

# machine properties
machinename=$1
diskcontroller=$2
diskport=0
diskdevice=0
machinedir="$HOME/.VirtualBox/Compressed/$machinename"
machinevbox="$machinedir/$machinename.vbox"
machineid=$(xmllint --xpath 'string(//*["Machine"]/@uuid)' "$machinevbox")
tempdir="/tmp/vbox-$UID/$machineid"
mkdir -p "$tempdir"
notify() {
notify-send -t 5000 -i "virtualbox" "$machinename" "$1"
}

if [ $(VBoxManage list runningvms | grep $machineid | wc -l) -eq 1 ]; then
  notify "Already running."
  exit
fi

if [ ! -f "$machinedir/Disk/$machinename.vdi" ]; then
  notify "Need permissions to mount squashfs."
  pkexec mount "$machinedir/$machinename.vdi.sfs" "$machinedir/Disk"
fi

if [ $(VBoxManage list vms | grep $machineid | wc -l) -eq 0 ]; then
  VBoxManage registervm "$machinevbox"
fi

VBoxManage modifyvm $machineid --snapshotfolder "$tempdir" 2>/dev/null

if [ -f "$machinedir/PrevStart.sfs" ]; then
  notify "Restoring previous saved state."
  unsquashfs -f -d "$tempdir" "$machinedir/PrevStart.sfs"
  rm "$machinedir/PrevStart.sfs"
elif [ -f "$machinedir/NewStart.sfs" ]; then
  notify "Using blank mutable overlay."
  unsquashfs -f -d "$tempdir" "$machinedir/NewStart.sfs"
  VBoxManage storageattach "$machinename" --storagectl $diskcontroller --port $diskport --device $diskdevice --type hdd --medium $tempdir/*.vdi
else
  notify "Creating blank mutable overlay."
  VBoxManage storageattach "$machinename" --storagectl $diskcontroller --port $diskport --device $diskdevice --type hdd --medium "$machinedir/Disk/$machinename.vdi"
  mksquashfs "$tempdir" "$machinedir/NewStart.sfs"
fi

# start machine
VirtualBoxVM --comment "$machinename" --startvm $machineid

if [ -f $tempdir/*.sav ]; then
  notify "Compressing saved state..."
  mksquashfs "$tempdir" "$machinedir/PrevStart.sfs"
  notify "Compressing saved state complete."
else
  VBoxManage storageattach "$machinename" --storagectl $diskcontroller --port $diskport --device $diskdevice --medium none
fi

# remove registration and temporary files
VBoxManage unregistervm $machineid
rm -r "$tempdir"
rm "$machinevbox-prev"
