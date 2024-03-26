/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 *   Copyright (c) 2016, 2018, University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *                         Sourjya Dutta <sdutta@nyu.edu>
 *                         Russell Ford <russell.ford@nyu.edu>
 *                         Menglei Zhang <menglei@nyu.edu>
 *
 *       Modified by: Tommaso Zugno <tommasozugno@gmail.com>
 *                               Integration of Carrier Aggregation
 */

#include "mmwave-enb-net-device.h"

#include "mmwave-net-device.h"
#include "mmwave-ue-net-device.h"

#include <ns3/abort.h>
#include <ns3/callback.h>
#include <ns3/enum.h>
#include <ns3/epc-x2.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/ipv6-l3-protocol.h>
#include <ns3/llc-snap-header.h>
#include <ns3/log.h>
#include <ns3/lte-enb-component-carrier-manager.h>
#include <ns3/lte-enb-rrc.h>
#include <ns3/lte-pdcp-tag.h>
#include <ns3/mmwave-component-carrier-enb.h>
#include <ns3/node.h>
#include <ns3/packet-burst.h>
#include <ns3/packet.h>
#include <ns3/pointer.h>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/uinteger.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MmWaveEnbNetDevice");

namespace mmwave
{

NS_OBJECT_ENSURE_REGISTERED(MmWaveEnbNetDevice);

TypeId
MmWaveEnbNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MmWaveEnbNetDevice")
            .SetParent<MmWaveNetDevice>()
            .AddConstructor<MmWaveEnbNetDevice>()
            .AddAttribute("LteEnbComponentCarrierManager",
                          "The ComponentCarrierManager associated to this EnbNetDevice",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_componentCarrierManager),
                          MakePointerChecker<LteEnbComponentCarrierManager>())
            .AddAttribute("LteEnbRrc",
                          "The RRC layer associated with the ENB",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::m_rrc),
                          MakePointerChecker<LteEnbRrc>())
            .AddAttribute("CellId",
                          "Cell Identifier",
                          UintegerValue(0),
                          MakeUintegerAccessor(&MmWaveEnbNetDevice::m_cellId),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("E2Termination",
                          "The E2 termination object associated to this node",
                          PointerValue(),
                          MakePointerAccessor(&MmWaveEnbNetDevice::SetE2Termination,
                                              &MmWaveEnbNetDevice::GetE2Termination),
                          MakePointerChecker<E2Termination>());
    return tid;
}

MmWaveEnbNetDevice::MmWaveEnbNetDevice()
    //: m_cellId(0),
    // m_Bandwidth (72),
    // m_Earfcn(1),
    : m_componentCarrierManager(0),
      m_isConfigured(false)
{
    NS_LOG_FUNCTION(this);
}

MmWaveEnbNetDevice::~MmWaveEnbNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
MmWaveEnbNetDevice::DoInitialize(void)
{
    NS_LOG_FUNCTION(this);
    m_isConstructed = true;
    UpdateConfig();
    for (auto it = m_ccMap.begin(); it != m_ccMap.end(); ++it)
    {
        it->second->Initialize();
    }
    m_rrc->Initialize();
    m_componentCarrierManager->Initialize();
}

void
MmWaveEnbNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_rrc->Dispose();
    m_rrc = 0;

    m_componentCarrierManager->Dispose();
    m_componentCarrierManager = 0;
    // MmWaveComponentCarrierEnb::DoDispose() will call DoDispose
    // of its PHY, MAC, FFR and scheduler instance
    for (uint32_t i = 0; i < m_ccMap.size(); i++)
    {
        m_ccMap.at(i)->Dispose();
        m_ccMap.at(i) = 0;
    }

    MmWaveNetDevice::DoDispose();
}

Ptr<MmWaveEnbPhy>
MmWaveEnbNetDevice::GetPhy(void) const
{
    NS_LOG_FUNCTION(this);
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(0))->GetPhy();
}

Ptr<MmWaveEnbPhy>
MmWaveEnbNetDevice::GetPhy(uint8_t index)
{
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(index))->GetPhy();
}

uint16_t
MmWaveEnbNetDevice::GetCellId() const
{
    NS_LOG_FUNCTION(this);
    return m_cellId;
}

bool
MmWaveEnbNetDevice::HasCellId(uint16_t cellId) const
{
    for (auto& it : m_ccMap)
    {
        if (DynamicCast<MmWaveComponentCarrierEnb>(it.second)->GetCellId() == cellId)
        {
            return true;
        }
    }
    return false;
}

uint8_t
MmWaveEnbNetDevice::GetBandwidth() const
{
    NS_LOG_FUNCTION(this);
    return m_Bandwidth;
}

void
MmWaveEnbNetDevice::SetBandwidth(uint8_t bw)
{
    NS_LOG_FUNCTION(this << bw);
    m_Bandwidth = bw;
}

Ptr<MmWaveEnbMac>
MmWaveEnbNetDevice::GetMac(void)
{
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(0))->GetMac();
}

Ptr<MmWaveEnbMac>
MmWaveEnbNetDevice::GetMac(uint8_t index)
{
    return DynamicCast<MmWaveComponentCarrierEnb>(m_ccMap.at(index))->GetMac();
}

void
MmWaveEnbNetDevice::SetRrc(Ptr<LteEnbRrc> rrc)
{
    m_rrc = rrc;
}

Ptr<LteEnbRrc>
MmWaveEnbNetDevice::GetRrc(void)
{
    return m_rrc;
}

Ptr<E2Termination>
MmWaveEnbNetDevice::GetE2Termination() const
{
    return m_e2term;
}

void
MmWaveEnbNetDevice::SetE2Termination(Ptr<E2Termination> e2term)
{
    m_e2term = e2term;

    NS_LOG_DEBUG("Register E2SM");

    Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    e2term->RegisterKpmCallbackToE2Sm(
        200,
        kpmFd,
        std::bind(&MmWaveEnbNetDevice::KpmSubscriptionCallback, this, std::placeholders::_1));
        
    // if (!m_forceE2FileLogging)
    // {
    //     Ptr<KpmFunctionDescription> kpmFd = Create<KpmFunctionDescription>();
    //     e2term->RegisterKpmCallbackToE2Sm(
    //         200,
    //         kpmFd,
    //         std::bind(&MmWaveEnbNetDevice::KpmSubscriptionCallback, this,
    //         std::placeholders::_1));
    // }
}

/**
 * KPM Subscription Request callback.
 * This function is triggered whenever a RIC Subscription Request for
 * the KPM RAN Function is received.
 *
 * \param pdu request message
 */
void
MmWaveEnbNetDevice::KpmSubscriptionCallback(E2AP_PDU_t* sub_req_pdu)
{
    NS_LOG_DEBUG("\nReceived RIC Subscription Request, cellId= " << m_cellId << "\n");

    E2Termination::RicSubscriptionRequest_rval_s params =
        m_e2term->ProcessRicSubscriptionRequest(sub_req_pdu);
    NS_LOG_DEBUG("requestorId " << +params.requestorId << ", instanceId " << +params.instanceId
                                << ", ranFuncionId " << +params.ranFuncionId << ", actionId "
                                << +params.actionId);

    // if (!m_isReportingEnabled)
    // {
    //     BuildAndSendReportMessage(params);
    //     m_isReportingEnabled = true;
    // }
}

bool
MmWaveEnbNetDevice::DoSend(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
    NS_ABORT_MSG_IF(protocolNumber != Ipv4L3Protocol::PROT_NUMBER &&
                        protocolNumber != Ipv6L3Protocol::PROT_NUMBER,
                    "unsupported protocol " << protocolNumber << ", only IPv4/IPv6 is supported");
    return m_rrc->SendData(packet);
    // EpcX2SapUser::UeDataParams params;

    // params.gtpTeid = 1;
    // params.ueData = packet;
    // PdcpTag pdcpTag(Simulator::Now());

    // params.ueData->AddByteTag(pdcpTag);
    // this->GetNode()->GetObject<EpcX2>()->GetX2RlcUserMap().find(1)->second->SendMcPdcpSdu(params);
}

void
MmWaveEnbNetDevice::UpdateConfig(void)
{
    NS_LOG_FUNCTION(this);

    if (m_isConstructed)
    {
        if (!m_isConfigured)
        {
            NS_LOG_LOGIC(this << " Configure cell " << m_cellId);
            // we have to make sure that this function is called only once
            // m_rrc->ConfigureCell (m_Bandwidth, m_Bandwidth, m_Earfcn, m_Earfcn, m_cellId);
            NS_ASSERT(!m_ccMap.empty());

            // create the MmWaveComponentCarrierConf map used for the RRC setup
            std::map<uint8_t, LteEnbRrc::MmWaveComponentCarrierConf> ccConfMap;
            for (auto it = m_ccMap.begin(); it != m_ccMap.end(); ++it)
            {
                Ptr<MmWaveComponentCarrierEnb> ccEnb =
                    DynamicCast<MmWaveComponentCarrierEnb>(it->second);
                LteEnbRrc::MmWaveComponentCarrierConf ccConf;
                ccConf.m_ccId = ccEnb->GetConfigurationParameters()->GetCcId();
                ccConf.m_cellId = ccEnb->GetCellId();
                ccConf.m_bandwidth = ccEnb->GetBandwidthInRb();

                ccConfMap[it->first] = ccConf;
            }

            m_rrc->ConfigureCell(ccConfMap);
            m_isConfigured = true;
        }

        // m_rrc->SetCsgId (m_csgId, m_csgIndication);
    }
    else
    {
        /*
         * Lower layers are not ready yet, so do nothing now and expect
         * ``DoInitialize`` to re-invoke this function.
         */
    }
}

void
MmWaveEnbNetDevice::SetCcMap(std::map<uint8_t, Ptr<MmWaveComponentCarrier>> ccm)
{
    NS_ASSERT_MSG(!m_isConfigured, "attempt to set CC map after configuration");
    m_ccMap = ccm;
}

} // namespace mmwave
} // namespace ns3
