from pathlib import Path
import html

out=Path(__file__).parent
s=['<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="1220" viewBox="0 0 1600 1220">', '<rect width="1600" height="1220" fill="#f3f6fa"/>', '<style>text{font-family:"PingFang SC","Microsoft YaHei",sans-serif;fill:#20354b}</style>']
def text(x,y,t,size=22,color='#20354b'):
    s.append(f'<text x="{x}" y="{y}" style="font-size:{size}px;fill:{color}">{html.escape(t)}</text>')
def box(x,y,w,h,color,stroke='none'):
    s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="14" fill="{color}" stroke="{stroke}"/>')
def line(points,color,dash=False):
    s.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in points)}" fill="none" stroke="{color}" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"'+(' stroke-dasharray="10 8"' if dash else '')+'/>')
def pad(x,y):
    s.append(f'<circle cx="{x}" cy="{y}" r="10" fill="#edcc78" stroke="#98712a" stroke-width="2"/>')
def dot(x,y):
    s.append(f'<circle cx="{x}" cy="{y}" r="8" fill="#394957"/>')
text(60,62,'门前设备 · 清晰接线图（不含电池）',36)
text(60,105,'XIAO ESP32-C3 + AM312 + 共阴红黄绿信号灯｜调试电源：USB-C',24)
box(35,135,1530,775,'#ffffff','#dce4ec')
# Board, USB at top. Physical header order matches XIAO ESP32-C3.
box(650,300,300,420,'#155d52')
box(745,278,110,65,'#c8d1d9','#8294a2')
text(759,316,'USB-C',21)
text(682,242,'USB线连接电脑供电',23)
line([(800,265),(800,200)],'#8796a5')
box(703,375,194,175,'#c6d0d6','#8b9da6')
text(728,425,'ESP32-C3',25)
text(756,464,'XIAO',29)
text(731,510,'正面 · USB朝上',19)
p={}
left=['D0','D1','D2','D3','D4','D5','D6']
right=['5V','GND','3V3','D10','D9','D8','D7']
for i,(a,b) in enumerate(zip(left,right)):
    yy=350+52*i
    pad(650,yy);pad(950,yy)
    text(666,yy+7,a,21,'#ffffff');text(892,yy+7,b,21,'#ffffff')
    p[a]=(650,yy);p[b]=(950,yy)
text(714,594,'未用引脚不接线',18,'#ffffff')
# PIR intentionally uses symbolic pads: actual physical order varies.
box(140,350,250,255,'#196c7a')
s.append('<circle cx="265" cy="410" r="43" fill="#edf1e7" stroke="#b6c5bc" stroke-width="2"/>')
text(209,490,'AM312',31,'#ffffff')
text(151,324,'人体红外感应模块',25)
pir={'VCC':(390,520),'OUT':(390,560),'GND':(390,600)}
for name,(x,y) in pir.items():
    pad(x,y);text(310,y+7,name,20,'#ffffff')
text(140,643,'引脚顺序按实物丝印确认',19)
# Traffic lights represented as four header pins with no fixed physical ordering implied.
box(1230,440,235,325,'#155d52')
text(1200,405,'共阴信号灯模块',25)
for yy,col,lab in [(500,'#e64b53','红'),(580,'#edbc42','黄'),(660,'#43b77c','绿')]:
    s.append(f'<circle cx="1390" cy="{yy}" r="24" fill="{col}"/>')
    text(1427,yy+7,lab,20,'#ffffff')
led={'G':(1230,506),'Y':(1230,662),'R':(1230,718),'GND':(1230,754)}
for name,(x,y) in led.items():
    pad(x,y);text(1250,y+7,name,20,'#ffffff')
text(1175,802,'模块须带限流电阻',20)
text(1175,833,'这里只示意端子，不代表排列',18)
# 3.3V via upper route; OUT from left. LED wires parallel direct routes.
line([p['3V3'],(1060,454),(1060,185),(470,185),(470,520),pir['VCC']],'#d44b47')
text(485,174,'① 3V3 → VCC',22,'#c03935')
line([pir['OUT'],(540,560),(540,454),p['D2']],'#1b91bc')
text(405,429,'② OUT → D2',22,'#167b9f')
line([p['D10'],led['G']],'#249961')
text(985,491,'③ D10 → G',20,'#20874f')
line([p['D7'],led['Y']],'#b8870d')
text(985,645,'④ D7 → Y',20,'#967008')
line([p['D6'],(610,662),(610,718),led['R']],'#db4f77')
text(985,705,'⑤ D6 → R',20,'#c93661')
# GND bus below all signal wires; junctions are explicitly dotted.
line([p['GND'],(1110,402),(1110,870)],'#394957')
line([pir['GND'],(440,600),(440,870),(1180,870),(1180,754),led['GND']],'#394957')
dot(1110,870)
text(600,850,'⑥ GND共地：AM312 GND + 信号灯GND',22,'#394957')
# Table.
box(35,935,1530,235,'#ffffff','#dce4ec')
text(65,977,'逐根核对',25)
rows=[('AM312 VCC → 3V3','AM312 OUT → D2 / GPIO4','AM312 GND → GND'),('信号灯 R → D6 / GPIO21','信号灯 Y → D7 / GPIO20','信号灯 G → D10 / GPIO10')]
for j,row in enumerate(rows):
    for i,t in enumerate(row):text(65+i*505,1020+j*42,t,22)
text(65,1110,'信号灯 GND → GND；G是绿灯输入，GND才是接地。线交叉无圆点表示不相连。',22)
text(65,1148,'AM312须支持3.3V供电；信号灯3.3V亮度待实测。改接线前拔掉USB。外设尚未实物验证。',20,'#965814')
text(60,1202,'按XIAO ESP32-C3引脚绘制；若开发板外形/丝印不同，请勿照图直接接线。',20)
s.append('</svg>')
svg='\n'.join(s)
(out/'front-wiring.svg').write_text(svg)
(out/'front-wiring.html').write_text('<!doctype html><meta charset="utf-8"><style>body{margin:0}svg{display:block}</style>'+svg)
print(out/'front-wiring.svg')
