/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "net-device.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NetDevice");

NS_OBJECT_ENSURE_REGISTERED(NetDevice);

TypeId
NetDevice::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NetDevice").SetParent<Object>().SetGroupName("Network");
    return tid;
}

NetDevice::~NetDevice()
{
    NS_LOG_FUNCTION(this);
}

bool NetDevice::IsQbb(void) const {
	return false;
}

bool NetDevice::SwitchSend (uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch){
	printf("NetDevice::SwitchSend not implemented\n");
	return false;
}

} // namespace ns3
