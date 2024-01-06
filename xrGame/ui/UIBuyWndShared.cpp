#include "stdafx.h"
#include "UIBuyWndShared.h"
#include "UIMPTradeWnd.h"

extern LPCSTR _list_names[];
extern u8 GetRanksCount();

void CItemMgr::Load(const shared_str &sect_cost)
{
	CInifile::Sect &sect = pSettings->r_section(sect_cost);
	const u32 _RANK_COUNT = GetRanksCount();

	for (const auto &it: sect.Data)
	{
		_i &val = m_items[it.first];
		val.slot_idx = u8(-1);
		val.cost.resize(_RANK_COUNT);

		std::string_view costsView = it.second.c_str();
		u32 idx = 0;

		while (true)
		{
			size_t commaPos = costsView.find(',');

			if (commaPos == std::string_view::npos)
				break;

			auto costStr = costsView.substr(0, commaPos);
			val.cost[idx++] = std::stoul(costStr.data());
			costsView = costsView.substr(commaPos + 1);
		}

		val.cost[idx++] = std::stoul(costsView.data());

		while (idx < _RANK_COUNT)
		{
			val.cost[idx] = val.cost[idx - 1];
			++idx;
		}
	}

	for (u8 i = CUIMpTradeWnd::e_first; i < CUIMpTradeWnd::e_total_lists; ++i)
	{
		const shared_str &buff = pSettings->r_string("buy_menu_items_place", _list_names[i]);

		u32 cnt = _GetItemCount(buff.c_str());
		string1024 _one;

		for (u32 c = 0; c < cnt; ++c)
		{
			_GetItem(buff.c_str(), c, _one);
			shared_str _one_str = _one;
			COST_MAP_IT it = m_items.find(_one_str);
			R_ASSERT(it != m_items.end());
			R_ASSERT3(it->second.slot_idx == u8(-1), "item has duplicate record in [buy_menu_items_place] section ", _one);
			it->second.slot_idx = i;
		}
	}
}

const u32 CItemMgr::GetItemCost(const shared_str &sect_name, u32 rank) const
{
	COST_MAP_CIT it = m_items.find(sect_name);
	VERIFY(it != m_items.end());
	return it->second.cost[rank];
}

const u8 CItemMgr::GetItemSlotIdx(const shared_str &sect_name) const
{
	COST_MAP_CIT it = m_items.find(sect_name);
	VERIFY(it != m_items.end());
	return it->second.slot_idx;
}

const u32 CItemMgr::GetItemIdx(const shared_str &sect_name) const
{
	COST_MAP_CIT it = m_items.find(sect_name);

	if (it == m_items.end())
	{
#ifdef DEBUG
		Msg("item not found in registry [%s]", sect_name.c_str());
#endif
		return u32(-1);
	}

	return u32(std::distance(m_items.begin(), it));
}

void CItemMgr::Dump() const
{
	COST_MAP_CIT it = m_items.begin();
	COST_MAP_CIT it_e = m_items.end();

	Msg("--CItemMgr::Dump");
	for (; it != it_e; ++it)
	{
		const _i &val = it->second;
		R_ASSERT3(it->second.slot_idx != u8(-1), "item has no record in [buy_menu_items_place] section ", it->first.c_str());
		Msg("[%s] slot=[%d] cost= %d,%d,%d,%d,%d", it->first.c_str(), val.slot_idx, val.cost[0], val.cost[1], val.cost[2], val.cost[3], val.cost[4]);
	}
}

const u32 CItemMgr::GetItemsCount() const
{
	return m_items.size();
}

const shared_str &CItemMgr::GetItemName(u32 Idx) const
{
	//R_ASSERT(Idx<m_items.size());

	if (Idx >= m_items.size())
	{
		Msg("! ERROR: cant get item index, invalid value [%u], max value [%u]", Idx, m_items.size());
		return (m_items.begin() + 1)->first;
	}

	return (m_items.begin() + Idx)->first;
}