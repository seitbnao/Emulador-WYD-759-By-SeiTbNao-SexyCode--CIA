/*
*   Copyright (C) {2015}  {Victor Klafke, Charles TheHouse}
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see [http://www.gnu.org/licenses/].
*
*   Contact at: victor.klafke@ecomp.ufsm.br
*/
#include "ProcessClientMessage.h"

void Exec_MSG_Buy(int conn, char *pMsg)
{
	MSG_Buy *m = (MSG_Buy*)pMsg;

	if (pMob[conn].MOB.CurrentScore.Hp == 0 || pUser[conn].Mode != USER_PLAY)
	{
		SendHpMode(conn);
		AddCrackError(conn, 10, 21);
		return;
	}

	if (pUser[conn].TradeMode)
	{
		RemoveTrade(pUser[conn].Trade.OpponentID);
		RemoveTrade(conn);
		return;
	}

	if (pUser[conn].Trade.OpponentID)
	{
		RemoveTrade(conn);
		return;
	}

	int TargetID = m->TargetID;
	int TargetInvenPos = m->TargetInvenPos;
	int MyInvenPos = m->MyInvenPos;

	if (TargetID < MAX_USER || TargetID >= MAX_MOB || TargetInvenPos < 0 || TargetInvenPos >= MAX_CARRY || MyInvenPos < 0 || MyInvenPos >= MAX_CARRY)
	{
		if (TargetID < MAX_USER && TargetID == conn)
		{
			if (TargetInvenPos > 27)
				return;

			if (TargetInvenPos < 0)
				return;

			if (TargetInvenPos >= 9)
				TargetInvenPos = 27;

			if (TargetInvenPos == 27)
				TargetInvenPos = 9;

			if (TargetInvenPos < 10)
			{
				auto My = pMob[conn].Rebuy[TargetInvenPos].Item;

				if (My.sIndex <= 0 || My.sIndex >= MAX_ITEMLIST)
					return;

				int szPrice = g_pItemList[My.sIndex].Price;

				if (szPrice > pMob[conn].MOB.Coin)
				{
					SendClientMessage(conn, g_pMessageStringTable[_NN_Havent_Money_So_Much]);

					return;
				}

				if (PutItem(conn, &My))
				{
					memset(&pMob[conn].Rebuy[TargetInvenPos].Item, 0, sizeof(STRUCT_ITEM));
					pMob[conn].Rebuy[TargetInvenPos].Price = 0;
					pMob[conn].Rebuy[TargetInvenPos].Ticks = 0;

					SendRecycle(conn);

					pMob[conn].MOB.Coin -= szPrice;

					SendEtc(conn);
				}
			}
			return;
		}
		//Log("err,MSG_BUY, target of carrypos outof range", pUser[conn].AccountName, pUser[conn].IP);
		//return;
	}

	if (pMob[TargetID].MOB.Merchant != 1)
	{
		SendClientMessage(conn, g_pMessageStringTable[_NN_Only_Merchant_Sells]);
		return;
	}
	if (!GetInView(conn, TargetID))
	{
		SendClientSignal(conn, ESCENE_FIELD, _MSG_CloseShop);
		return;
	}

	int itemIndex = pMob[TargetID].MOB.Carry[TargetInvenPos].sIndex;

	STRUCT_ITEM *ItemMob = &pMob[TargetID].MOB.Carry[TargetInvenPos];

	unsigned int Donate = BASE_GetItemAbility(ItemMob, EF_DONATE);
	unsigned int Honra = BASE_GetItemAbility(ItemMob, EF_HONRA);

	if (itemIndex <= 0 || itemIndex >= MAX_ITEMLIST)
		return;

	if (strcmp(pMob[TargetID].MOB.MobName, "Fame FoemaSB") == 0 || 
		strcmp(pMob[TargetID].MOB.MobName, "Fame BeastSB") == 0 || 
		strcmp(pMob[TargetID].MOB.MobName, "Fame HunterSB") == 0 ||
		strcmp(pMob[TargetID].MOB.MobName, "Fame TransSB") == 0)
	{
		if (itemIndex >= 5500 && itemIndex <= 5547)
		{
			if (pMob[conn].extra.Fame < 2000)
			{
				SendClientMessage(conn, "Necess�rio 2.000 de fame para prosseguir!");
				return;
			}
			else
			{
				pMob[conn].extra.Fame -= 2000;

				char tmplog[2048];
				BASE_GetItemCode(ItemMob, tmplog);

				sprintf(temp, "buy_npc,npc:%s price:%d item:%s", pMob[TargetID].MOB.MobName, 2000, tmplog);
				ItemLog(temp, pUser[conn].AccountName, pUser[conn].IP);

				SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
			}
		}
	}
	 
		/* Donate funciona puxando o id da gener do npc */
		if (Donate)
		{
			if (Donate > pUser[conn].Donate)
			{
				sprintf(temp, g_pMessageStringTable[_DN_NeedDonate], Donate);
				SendClientMessage(conn, temp);
				return;
			}

			if (itemIndex == 0)
			{
				printf(temp, g_pMessageStringTable[_DN_NeedDonate], Donate);
				Log("err,buy request null item from shop donate - MSG_BUY", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}

			if (pMob[conn].MOB.Carry[m->MyInvenPos].sIndex != 0)
			{
				SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
				return;
			}

			int pos = 0;

			for (pos = 0; pos < pMob[conn].MaxCarry && pos < MAX_CARRY; pos++)
			{
				if (pMob[conn].MOB.Carry[pos].sIndex == 0)
					break;
			}

			if (pos >= 0 && pos < pMob[conn].MaxCarry && pos < MAX_CARRY)
			{
				memcpy(&pMob[conn].MOB.Carry[MyInvenPos], ItemMob, sizeof(STRUCT_ITEM));
				pUser[conn].Donate -= Donate;
				m->ID = ESCENE_FIELD;
				pUser[conn].cSock.AddMessage((char*)m, m->Size);
				SendEtc(conn);
				SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
			}

			else
			{
				SendClientMessage(conn, g_pMessageStringTable[_NN_You_Have_No_Space_To_Trade]);
				return;
			}

			sprintf(temp, "Comprou o item [ %s ] por [ %d] restam [ %d ] de Donate", g_pItemList[itemIndex].Name, Donate, pUser[conn].Donate);
			SendClientMessage(conn, temp);


			sprintf(temp, "%s comprou o item %s por %d donate e restou %d de Donate", pMob[conn].MOB.MobName, g_pItemList[itemIndex].Name, Donate, pUser[conn].Donate);
			ItemLog(temp, pUser[conn].AccountName, pUser[conn].IP);
		}

		if (Honra)
		{
			if (Honra > pUser[conn].Honra)
			{
				sprintf(temp, "Honra Insuficiente!", Honra);
				SendClientMessage(conn, temp);
				return;
			}

			if (itemIndex == 0)
			{
				sprintf(temp, "Honra Insuficiente!", Honra);
				Log("err,buy request null item from shop honra - MSG_BUY", pUser[conn].AccountName, pUser[conn].IP);
				return;
			}

			if (pMob[conn].MOB.Carry[m->MyInvenPos].sIndex != 0)
			{
				SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
				return;
			}

			int pos = 0;

			for (pos = 0; pos < pMob[conn].MaxCarry && pos < MAX_CARRY; pos++)
			{
				if (pMob[conn].MOB.Carry[pos].sIndex == 0)
					break;
			}

			if (pos >= 0 && pos < pMob[conn].MaxCarry && pos < MAX_CARRY)
			{
				memcpy(&pMob[conn].MOB.Carry[MyInvenPos], ItemMob, sizeof(STRUCT_ITEM));
				pUser[conn].Honra -= Honra;
				m->ID = ESCENE_FIELD;
				pUser[conn].cSock.AddMessage((char*)m, m->Size);
				SendEtc(conn);
				SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
			}

			else
			{
				SendClientMessage(conn, g_pMessageStringTable[_NN_You_Have_No_Space_To_Trade]);
				return;
			}

			sprintf(temp, "Comprou o item [ %s ] por [ %d] restam [ %d ] de Honra", g_pItemList[itemIndex].Name, Honra, pUser[conn].Honra);
			SendClientMessage(conn, temp);


			sprintf(temp, "%s comprou o item %s por %d honra e restou %d de Honra", pMob[conn].MOB.MobName, g_pItemList[itemIndex].Name, Honra, pUser[conn].Honra);
			ItemLog(temp, pUser[conn].AccountName, pUser[conn].IP);
		}

	int Price = g_pItemList[itemIndex].Price;
	int Price2 = Price;
	int Village = BASE_GetVillage(pMob[TargetID].TargetX, pMob[TargetID].TargetY);
	int CityTax = g_pGuildZone[Village].CityTax;

	if (Village < 0 || Village >= 5 || CityTax <= 0 || CityTax >= 30)
		goto LABEL_BUY1;

	if (Price < 100000)
		Price += Price * CityTax / 100;

	else
		Price += Price * CityTax / 100;

	if (Price < 0)
		return;

	int GuildTax = (Price - Price2) / 2;
LABEL_BUY1:

	int bPrice = Price;
	int Desconto = 0;
	int TargetVillage = BASE_GetVillage(pMob[TargetID].TargetX, pMob[TargetID].TargetY);
	int Guild = pMob[conn].MOB.Guild;

	if (Price <= 1999999999 && TargetVillage >= 0 && TargetVillage < 5 && Guild > 0 && g_pGuildZone[TargetVillage].ChargeGuild == Guild)
	{
		Desconto = 30;
		Price = 7 * Price / 10;
	}

	if (Price <= 1999999999 && !Desconto && pMob[conn].MOB.Class == 3 && pMob[conn].MOB.LearnedSkill & 0x800)
	{
		int special = (pMob[conn].MOB.CurrentScore.Special[2] / 10) + 6;

		if ((pMob[conn].MOB.CurrentScore.Special[2] / 10) + 6 >= 26)
			special = 26;

		Desconto = special;
		special /= 2;

	}

	if (Price > pMob[conn].MOB.Coin)
	{
		SendClientMessage(conn, g_pMessageStringTable[_NN_Not_Enough_Money]);
		return;
	}

	if (itemIndex == 0)
	{
		Log("err,buy request null item from shop - MSG_BUY", pUser[conn].AccountName, pUser[conn].IP);
		return;
	}

	if (pMob[conn].MOB.Carry[m->MyInvenPos].sIndex != 0)
	{
		SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
		return;
	}

	if (itemIndex == 508 || itemIndex == 509 || itemIndex == 446 || itemIndex >= 526 && itemIndex <= 531)
	{
		if (ServerIndex == -1)
		{
			Log("err,Buy Guild item, but, Server index undefined", pUser[conn].AccountName, pUser[conn].IP);
			return;
		}

		if (!GuildCounter)
		{
			Log("err,Buy Guild item, but, Guild counter zero", pUser[conn].AccountName, pUser[conn].IP);
			return;
		}

		if (itemIndex == 508 || itemIndex == 446 || itemIndex >= 526 && itemIndex <= 531)
		{
			if (!pMob[conn].MOB.GuildLevel)
			{
				SendClientMessage(conn, g_pMessageStringTable[114]);
				return;
			}
			if (pMob[conn].MOB.GuildLevel < 2 && itemIndex >= 526 && itemIndex <= 531)
			{
				SendClientMessage(conn, g_pMessageStringTable[114]);
				return;
			}
			ItemMob->stEffect[0].cEffect = 56;
			ItemMob->stEffect[0].cValue = Guild;
			ItemMob->stEffect[1].cEffect = 57;
			ItemMob->stEffect[1].cValue = Guild;

			if (itemIndex == 508)
			{
				int sub = BASE_GetSubGuild(pMob[conn].MOB.Equip[12].sIndex);

				ItemMob->stEffect[2].cEffect = 85;
				ItemMob->stEffect[2].cValue = sub;

				if (sub >= 1 && sub <= 3)
					ItemMob->sIndex = sub + 531;
			}
		}

		if (itemIndex == 509)
		{
			ItemMob->stEffect[0].cEffect = 56;
			ItemMob->stEffect[0].cValue = (GuildCounter + (ServerIndex << 12)) >> 8;

			ItemMob->stEffect[1].cEffect = 57;
			ItemMob->stEffect[1].cValue = GuildCounter;

			ItemMob->stEffect[2].cEffect = 59;
			ItemMob->stEffect[2].cValue = rand();

			int GCount = GuildCounter + (ServerIndex << 12);

			sprintf(temp, "sys,guild medal value:%d count:%d", GCount, GuildCounter);
			Log(temp, pUser[conn].AccountName, pUser[conn].IP);

			GuildCounter++;

			if (GuildCounter >= 4096)
			{
				GuildCounter = 2048;
				Log("err,GuildCounter Restarted.", pUser[conn].AccountName, pUser[conn].IP);
			}
			CReadFiles::WriteGuild();
		}
	}

	if (itemIndex >= 2300 && itemIndex < 2330)
	{
		ItemMob->stEffect[1].cEffect = EF_INCUBATE;
		ItemMob->stEffect[1].cValue = (rand() % 7 + 1);
	}

	if (Price >= 0)
	{
		int x = 0;

		for (x = 0; x < MAX_MOB_MERC; x++)
		{
			//Verifica se o vendedor � um mob mercador especial.
			if (pMob[TargetID].GenerateIndex == pMobMerc[x].GenerateIndex)
				break;
		}

		if (x != MAX_MOB_MERC)
		{//O Vendedor � um mob mercador especial, portanto ele verifica se ainda possui unidades daquele item 
		 //se tiver ele diminui do stock caso contr�rio ele avisa que n�o tem.
			if (pMobMerc[x].Stock[TargetInvenPos] == 0)
			{
				SendClientMessage(conn, g_pMessageStringTable[_NN_NOSTOCK]);
				return;
			}
			pMobMerc[x].Stock[TargetInvenPos]--;
		}

		if (TargetVillage >= 0 && TargetVillage < 5 && CityTax > 0 && CityTax < 30)
		{
			if (GuildImpostoID[TargetVillage] >= MAX_USER && GuildImpostoID[TargetVillage] < MAX_MOB && pMob[GuildImpostoID[TargetVillage]].Mode != USER_EMPTY && pMob[GuildImpostoID[TargetVillage]].MOB.Guild == g_pGuildZone[TargetVillage].ChargeGuild)
			{
				if (pMob[GuildImpostoID[TargetVillage]].MOB.Exp < 200000000000)
					pMob[GuildImpostoID[TargetVillage]].MOB.Exp += GuildTax / 2;


			}
			if (GuildImpostoID[4] >= MAX_USER && GuildImpostoID[4] < MAX_MOB && GuildTax > 0 && pMob[GuildImpostoID[4]].Mode != USER_EMPTY && pMob[GuildImpostoID[4]].MOB.Guild == g_pGuildZone[4].ChargeGuild)
			{
				if (pMob[GuildImpostoID[4]].MOB.Exp < 200000000000)
					pMob[GuildImpostoID[4]].MOB.Exp += GuildTax;

			}
		}

		if (MyInvenPos >= 0 && MyInvenPos < pMob[conn].MaxCarry && MyInvenPos < MAX_CARRY)
			pMob[conn].MOB.Coin -= Price;

		m->Coin = pMob[conn].MOB.Coin;

		m->ID = ESCENE_FIELD;

		pUser[conn].cSock.AddMessage((char*)m, m->Size);

		SendEtc(conn);
	DonateBuy:

		if (MyInvenPos >= 0 && MyInvenPos < pMob[conn].MaxCarry && MyInvenPos < MAX_CARRY)
		{
			memcpy(&pMob[conn].MOB.Carry[MyInvenPos], ItemMob, sizeof(STRUCT_ITEM));

			//PutItem(conn, ItemMob);

			char tmplog[2048];
			BASE_GetItemCode(ItemMob, tmplog);

			sprintf(temp, "buy_npc,npc:%s price:%d item:%s", pMob[TargetID].MOB.MobName, Price, tmplog);
			ItemLog(temp, pUser[conn].AccountName, pUser[conn].IP);

			SendItem(conn, ITEM_PLACE_CARRY, m->MyInvenPos, &pMob[conn].MOB.Carry[m->MyInvenPos]);
		}
		else
		{ 
			SendMsgExp(conn, g_pMessageStringTable[_NN_You_Have_No_Space_To_Trade], TNColor::Red, false);
			return;
		}

		if (FREEEXP == -2)
		{
			sprintf(temp, "%s base:%d tax:%d(%d%%) discount:%d(%d%%)", g_pItemList[itemIndex].Name, Price2, bPrice, CityTax, Price, Desconto);
			SendSay(TargetID, temp);
		}

		if (itemIndex == 787 || itemIndex == 481)
		{
			pMob[TargetID].PotionCount++;

			if (pMob[TargetID].PotionCount > POTIONCOUNT)
				DeleteMob(TargetID, 3);
		}
		if (pMob[TargetID].MOB.Equip[0].sIndex == 259 || pMob[TargetID].MOB.Equip[0].sIndex == 230)
			DeleteMob(TargetID, 3);
	}
	else
		Log("err,MSG_ReqBuy  <><><><><><> MINUS <><><><><><>", pUser[conn].AccountName, pUser[conn].IP);
}