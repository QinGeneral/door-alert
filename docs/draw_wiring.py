from pathlib import Path
import html

out = Path(__file__).parent
s = ['<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="1500" viewBox="0 0 1600 1500">', '<rect width="1600" height="1500" fill="#f2f5f9"/>', '<style>text{font-family:"PingFang SC","Microsoft YaHei",sans-serif;fill:#1d3046} .small{font-size:18px}.label{font-size:22px}.title{font-size:32px;font-weight:700}</style>']
def rect(x,y,w,h,fill,stroke='none',r=12):
    s.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{stroke}"/>')
def txt(x,y,t,size=22,color='#1d3046'):
    s.append(f'<text x="{x}" y="{y}" style="font-size:{size}px;fill:{color}">{html.escape(t)}</text>')
def wire(points,color,dash=False):
    s.append(f'<polyline points="{" ".join(f"{x},{y}" for x,y in points)}" fill="none" stroke="{color}" stroke-width="5" stroke-linejoin="round" stroke-linecap="round"'+(' stroke-dasharray="10 8"' if dash else '')+'/>')
def pad(x,y):
    s.append(f'<circle cx="{x}" cy="{y}" r="9" fill="#e8c66a" stroke="#805f20" stroke-width="2"/>')
def board(x,y):
    rect(x,y,300,370,'#135c50')
    rect(x+100,y-18,100,65,'#c6cdd4','#657684',5)
    txt(x+107,y+20,'USB-C',18)
    rect(x+64,y+85,172,160,'#c7ced3','#8d969b',6)
    txt(x+86,y+133,'ESP32-C3',22)
    txt(x+82,y+168,'XIAO',26)
    txt(x+84,y+202,'正面 / USB朝上',16)
    p={}
    left=[('D0',2),('D1',3),('D2',4),('D3',5),('D4',6),('D5',7),('D6',21)]
    right=[('5V',None),('GND',None),('3V3',None),('D10',10),('D9',9),('D8',8),('D7',20)]
    for i,((a,g),(b,h)) in enumerate(zip(left,right)):
        yy=y+47+i*46
        pad(x,yy);pad(x+300,yy)
        txt(x+15,yy+6,a,18,'#ffffff');txt(x+248,yy+6,b,18,'#ffffff')
        p[a]=(x,yy);p[b]=(x+300,yy)
    return p

txt(55,60,'门后有人提示器 · 板级接线图',34)
txt(55,96,'按 Seeed XIAO ESP32-C3 绘制｜外设引脚位置为示意，接线认丝印｜未进行外设实物验证',21)
rect(35,125,1530,560,'#ffffff','#d9e1eb')
txt(65,169,'① 门后：USB供电 + VL53L5CX',28)
p=board(330,235)
txt(300,215,'接电脑 / USB充电头',20)
rect(1050,245,340,275,'#176b7a')
rect(1150,275,140,100,'#172e40')
txt(1085,414,'VL53L5CX 模块',26,'#ffffff')
for name,yy in [('SDA',450),('SCL',492)]:
    pad(1050,yy);txt(1070,yy+6,name,20,'#ffffff')
pad(1390,450);txt(1308,456,'GND',20,'#ffffff')
pad(1390,492);txt(1308,498,'VIN*',20,'#ffffff')
wire([p['D4'],(220,p['D4'][1]),(220,570),(900,570),(900,450),(1050,450)],'#2377d2')
wire([p['D5'],(265,p['D5'][1]),(265,605),(950,605),(950,492),(1050,492)],'#9a59ca')
wire([p['GND'],(1460,p['GND'][1]),(1460,450),(1390,450)],'#394350')
txt(760,560,'蓝：D4 / GPIO6 → SDA',18,'#2377d2')
txt(760,632,'紫：D5 / GPIO7 → SCL',18,'#9a59ca')
wire([(1390,492),(1495,492),(1495,540)],'#d68719',True)
txt(1030,655,'* VIN电压待核实：供电线暂不接；确认模块规格后再接3V3或5V。',18,'#a15d06')

rect(35,710,1530,640,'#ffffff','#d9e1eb')
txt(65,754,'② 门前：锂电池 + AM312 + 红黄绿信号灯',28)
p=board(400,865)
rect(80,825,210,175,'#176b7a')
s.append('<circle cx="185" cy="875" r="35" fill="#eef1e6" stroke="#b9c3bd"/>')
txt(128,938,'AM312',25,'#ffffff')
for name,xx in [('VCC',110),('OUT',185),('GND',260)]:
    pad(xx,1000);txt(xx-22,978,name,16,'#ffffff')
wire([(185,1000),(185,1004),(350,1004),p['D2']],'#22a5b3')
wire([p['3V3'],(755,p['3V3'][1]),(755,805),(110,805),(110,1000)],'#df4949')
wire([(260,1000),(260,1280),(1450,1280),(1450,p['GND'][1]),p['GND']],'#394350')
rect(1060,840,300,340,'#135c50')
for color,yy,t in [('#e74848',900,'红：检测到目标'),('#edb83e',985,'黄：状态未知'),('#4aba7c',1070,'绿：未检测到')]:
    s.append(f'<circle cx="1120" cy="{yy}" r="23" fill="{color}"/>')
    txt(1160,yy+7,t,17,'#ffffff')
for name,yy in [('R',875),('Y',960),('G',1045),('GND',1135)]:
    pad(1060,yy);txt(1077,yy+6,name,16,'#ffffff')
wire([p['D6'],(330,p['D6'][1]),(330,1250),(810,1250),(810,875),(1060,875)],'#db4f6a')
wire([p['D7'],(855,p['D7'][1]),(855,960),(1060,960)],'#c48f06')
wire([p['D10'],(900,p['D10'][1]),(900,1045),(1060,1045)],'#24995d')
wire([(1060,1135),(1000,1135),(1000,1280)],'#394350')
s.append('<circle cx="1000" cy="1280" r="7" fill="#394350"/>')
txt(750,1210,'R=D6/GPIO21 · Y=D7/GPIO20 · G=D10/GPIO10',17)
txt(82,1036,'OUT → D2 / GPIO4',18,'#158e9a')
rect(72,1080,200,130,'#d2d9e2','#899aaa')
txt(91,1117,'单节 3.7V',23)
txt(91,1150,'1000mAh',23)
txt(91,1182,'带保护板',19)
wire([(272,1110),(300,1110),(300,1308),(470,1308)],'#df4949')
wire([(272,1170),(285,1170),(285,1330),(650,1330)],'#394350')
txt(485,1314,'BAT+（背面焊盘）',16,'#c63030')
txt(665,1336,'BAT−（背面焊盘）',16)
txt(65,1387,'信号灯须为带限流电阻的共阴模块；3.3V驱动亮度需实测。GPIO不得输入5V。',21)
txt(65,1423,'电池接BAT焊盘，不接5V脚。模块供电、极性及引脚排列须按实物确认；改接线前断开USB和电池。',20)
txt(65,1460,'两块ESP32之间通过ESP-NOW无线通信，无需接线或共地。本图不含可选电量检测分压电路。',20)
s.append('</svg>')
(out/'wiring.svg').write_text('\n'.join(s))
(out/'wiring.html').write_text('<!doctype html><meta charset="utf-8"><style>body{margin:0}svg{display:block}</style>'+ '\n'.join(s))
print(out/'wiring.svg')
