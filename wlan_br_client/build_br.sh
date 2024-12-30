#!/bin/bash
ACTION=$1
export LD_LIBRARY_PATH=$(pwd)/../src_lib/libcli/
BR_NAME=wlan_br
VETH_NAME=veth
VETH_NAME_0="${VETH_NAME}0"
VETH_NAME_1="${VETH_NAME}1"
TAP_NAME=ttap0
VETH_IP=6.6.6.137/24
SERVER_IP=172.16.123.137
function build_veth()
{
	ip link add ${VETH_NAME_0} type veth peer name ${VETH_NAME_1}
	ip link set ${VETH_NAME_0} up
	ip link set ${VETH_NAME_1} up
	ip addr add ${VETH_IP} dev ${VETH_NAME_0}
	echo "[shell note] create ${VETH_NAME_0}/${VETH_NAME_1} success!"
}

function del_veth()
{

	ip link set ${VETH_NAME_0} down
	ip link set ${VETH_NAME_1} down
	ip link del ${VETH_NAME_0} type veth peer name ${VETH_NAME_1}
}

TUNTAP_APP_NAME=wlan_br_client
TUNTAP_APP_PATH=$(pwd)/bin/${TUNTAP_APP_NAME}

function build_tuntap()
{
	if [ -z "$(pidof ${TUNTAP_APP_NAME})" ];then
		${TUNTAP_APP_PATH} -s ${SERVER_IP}&
		echo "[shell note] create ${TUNTAP_APP_NAME} success!"
	fi
}

function del_tuntap()
{
	if [ ! -z "$(pidof ${TUNTAP_APP_NAME})" ];then
		killall -10 ${TUNTAP_APP_NAME}
	fi
	ip link del dev ${TAP_NAME}
	
}

function build_br()
{
	ip link add name ${BR_NAME} type bridge
	ip link set ${TAP_NAME} master ${BR_NAME}
	ip link set ${VETH_NAME_1} master ${BR_NAME}

	ip link set ${BR_NAME} up
	ip link set ${TAP_NAME} up
	ip link set ${VETH_NAME_1} up
	echo "[shell note] create ${BR_NAME} success!"
	ip link show master ${BR_NAME}
}

function del_br()
{
	ip link set ${TAP_NAME} nomaster
	ip link set ${VETH_NAME_1} nomaster
	ip link set ${BR_NAME} down
	ip link set ${TAP_NAME} down
	ip link del dev ${BR_NAME}

}

function main()
{
	ACTION=$1
	if [ "${ACTION}" == "add" ];then
		build_veth;
		build_tuntap;
		build_br;
	elif [ "${ACTION}" == "del" ];then
		del_br;
		del_tuntap;
		del_veth;
	else
		echo "[Error] please input argument!"
		echo "eg:"
		echo "\t$0 add    --- add bridge, veth, tuntap nic"
		echo "\t$0 del    --- add bridge, veth, tuntap nic"
	fi
}


main $@
