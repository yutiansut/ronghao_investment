策略描述：
观察卖出价：昨高+0.35×（昨收-昨低）
反转卖出价：（1.07/2）×（昨高+昨低）-0.07×昨低
反转买入价：（1.07/2）×（昨高+昨低）-0.07×昨高

观察买入价：昨低-0.35×（昨高-昨收）
突破买入价：观察卖出价+0.25×（观察卖出价-观察买入价）
突破卖出价：观察买入价-0.25×（观察卖出价-观察买入价）

开仓：	价格超过突破买入价		开多
			价格跌破突破卖出价		开空
			
持多单：当日内最高价超过观察卖出价后，盘中出现回落，且进一步跌破反转卖出价构成的支撑线时，反手做空
持空单：当日内最低价低于观察买入价后，盘中价格反弹，且进一步超过反转买入价构成的阻力线后，反手做多