from pathlib import Path
import html

out=Path(__file__).parent
s=['<svg xmlns="http://www.w3.org/2000/svg" width="1500" height="1060" viewBox="0 0 1500 1060">','<rect width="1500" height="1060" fill="#f3f6fa"/>','<style>text{font-family:"PingFang SC","Microsoft YaHei",sans-serif;fill:#20354b}</style>']
def text(x,y,t,size=22,color='#20354b'):
 s.append(f'<text x="{x}" y="{y}" style="font-size:{size}px;fill:{color}">{html.escape(t)}</text>')
def box(x,y,w,h,c,stroke='none'):
 s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="14" fill="{c}" stroke="{stroke}"/>')
def line(p,c,dash=False):
 pts=" ".join(f"{x},{y}" for x,y in p)
 dash=' stroke-dasharray="12 9"' if dash else ''
 s.append(f'<polyline points="{pts}" fill="none" stroke="{c}" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"{dash}/>')
def pad(x,y):
 s.append(f'<circle cx="{x}" cy="{y}" r="10" fill="#edcc78" stroke="#98712a" stroke-width="2"/>')
text(55,60,'门后设备 · 按实物丝印更新',36)
text(55,103,'XIAO ESP32-C3 + VL53L5CX 测距模块｜USB-C供电｜不含雷达',24)
box(30,135,1440,650,'#ffffff','#dce4ec')
# Header locations: board face up, USB at top.
box(350,270,300,420,'#155d52')
box(445,248,110,65,'#c8d1d9','#8294a2')
text(460,286,'USB-C',21)
text(365,205,'电脑 / USB充电头',23)
line([(500,222),(500,244)],'#8898a6')
box(405,365,190,160,'#c6d0d6','#8b9da6')
text(428,415,'ESP32-C3',25)
text(458,454,'XIAO',29)
text(427,496,'正面 · USB朝上',18)
p={}
for i,(a,b) in enumerate(zip(['D0','D1','D2','D3','D4','D5','D6'],['5V','GND','3V3','D10','D9','D8','D7'])):
 y=320+i*52;pad(350,y);pad(650,y)
 text(367,y+7,a,21,'#ffffff');text(590,y+7,b,21,'#ffffff')
 p[a]=(350,y);p[b]=(650,y)
# Header order observed in IMG_8154: front face up, VIN at the top.
box(1060,310,330,380,'#196c7a')
box(1230,425,105,155,'#213a4c')
text(1040,290,'VL53L5CX · 传感器正面朝上',23)
for name,y in [('VIN',350),('GND',410),('SCL',470),('SDA',530),('INT',590),('LPn',650)]:
 pad(1060,y);text(1080,y+7,name,21,'#ffffff')
line([p['3V3'],(760,424),(760,230),(1000,230),(1000,350),(1060,350)],'#d64d49')
text(775,214,'① 3V3 → VIN*',23,'#c33935')
line([p['GND'],(900,372),(900,410),(1060,410)],'#394957')
text(705,358,'② GND → GND',22,'#394957')
line([p['D4'],(250,528),(250,725),(820,725),(820,530),(1060,530)],'#2679d6')
text(868,518,'③ D4 → SDA',21,'#2679d6')
line([p['D5'],(300,580),(300,760),(875,760),(875,470),(1060,470)],'#9854be')
text(905,458,'④ D5 → SCL',21,'#9854be')
text(1150,597,'暂不连接',18,'#ffffff')
text(1150,657,'暂不连接*',18,'#ffffff')
text(1000,739,'本图采用3.3V联调；5V脚不接。',21,'#a26210')
box(30,810,1440,205,'#ffffff','#dce4ec')
text(60,854,'逐根核对',25)
text(60,896,'3V3 → VIN',24,'#c33935')
text(410,896,'GND → GND',24)
text(760,896,'D4 → SDA',24,'#2679d6')
text(1110,896,'D5 → SCL',24,'#9854be')
text(60,941,'* 电源为3.3V初步联调方案，不能仅凭照片确认整板额定参数；勿改接5V。',22,'#a26210')
text(60,980,'INT 与 LPn 留空。VIN 只接 3V3，不要改接 5V。',21)
text(55,1045,'两块主板之间使用ESP-NOW无线通信，不接导线、不共地；改接线前断开USB。',21)
s.append('</svg>')
svg='\n'.join(s)
(out/'rear-wiring.svg').write_text(svg)
(out/'rear-wiring.html').write_text('<!doctype html><meta charset="utf-8"><style>body{margin:0}svg{display:block}</style>'+svg)
print(out/'rear-wiring.svg')
