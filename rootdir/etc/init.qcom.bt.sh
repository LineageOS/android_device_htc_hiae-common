#!/system/bin/sh

LOG_TAG="qcom-bluetooth"
LOG_NAME="${0}:"

hciattach_pid=""

loge ()
{
  /system/bin/log -t $LOG_TAG -p e "$LOG_NAME $@"
}

logi ()
{
  /system/bin/log -t $LOG_TAG -p i "$LOG_NAME $@"
}

start_hciattach ()
{
  /system/bin/hciattach -n $BTS_DEVICE $BTS_TYPE $BTS_BAUD &
  hciattach_pid=$!
  logi "start_hciattach: pid = $hciattach_pid"
}

kill_hciattach ()
{
  logi "kill_hciattach: pid = $hciattach_pid"
  ## careful not to kill zero or null!
  kill -TERM $hciattach_pid
  # this shell doesn't exit now -- wait returns for normal exit
}

setprop bluetooth.status off

# default BT LE power class 2 (-P 1)

# Note that "hci_qcomm_init -e" prints expressions to set the shell variables
# BTS_DEVICE, BTS_TYPE, BTS_BAUD, and BTS_ADDRESS.
eval $(/system/bin/hci_qcomm_init -e -P 1 && echo "exit_code_hci_qcomm_init=0" || echo "exit_code_hci_qcomm_init=1")
case $exit_code_hci_qcomm_init in
  0)
      logi "Bluetooth QSoC firmware download succeeded, $BTS_DEVICE $BTS_TYPE $BTS_BAUD $BTS_ADDRESS"
      ;;
  *)
      loge "Bluetooth QSoC firmware download failed"
      setprop bluetooth.status off
      exit $exit_code_hci_qcomm_init
      ;;
esac

# init does SIGTERM on ctl.stop for service
trap "kill_hciattach" TERM INT

setprop bluetooth.status on

exit 0
