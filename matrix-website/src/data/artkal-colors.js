// Artkal 完整221色数据库
// 数据来源：Artkal官方221色色卡

// 完整颜色数据
const ARTKAL_COLORS_FULL = [
  { code: 'A1', name: '淡鹅黄', hex: '#FAF4C8', group: '1' },
  { code: 'A2', name: '奶油白', hex: '#FFFFD5', group: '1' },
  { code: 'A3', name: '柠檬黄', hex: '#FEFF8B', group: '1' },
  { code: 'A4', name: '亮黄', hex: '#FBED56', group: '1' },
  { code: 'A5', name: '鲜黄', hex: '#F4D738', group: '1' },
  { code: 'A6', name: '金黄', hex: '#FEAC4C', group: '1' },
  { code: 'A7', name: '橙黄', hex: '#FE8B4C', group: '1' },
  { code: 'A8', name: '土黄', hex: '#FFDA45', group: '1' },
  { code: 'A9', name: '古金', hex: '#FF995B', group: '1' },
  { code: 'A10', name: '肉橘', hex: '#F77C31', group: '1' },
  { code: 'A11', name: '杏色', hex: '#FFDD99', group: '1' },
  { code: 'A12', name: '浅橙粉', hex: '#FE9F72', group: '1' },
  { code: 'A13', name: '橘黄', hex: '#FFC365', group: '1' },
  { code: 'A14', name: '亮橙红', hex: '#FD543D', group: '1' },
  { code: 'A15', name: '淡黄', hex: '#FFF365', group: '1' },
  { code: 'A16', name: '亮黄', hex: '#FFFF9F', group: '1' },
  { code: 'A17', name: '浅金黄', hex: '#FFE36E', group: '1' },
  { code: 'A18', name: '珊瑚橘', hex: '#FEBE7D', group: '1' },
  { code: 'A19', name: '亮粉红', hex: '#FD7C72', group: '1' },
  { code: 'A20', name: '淡橘黄', hex: '#FFD568', group: '1' },
  { code: 'A21', name: '淡金色', hex: '#FFE395', group: '1' },
  { code: 'A22', name: '亮黄绿', hex: '#F4F57D', group: '1' },
  { code: 'A23', name: '肤色', hex: '#E6C9B7', group: '1' },
  { code: 'A24', name: '浅黄', hex: '#F7F8A2', group: '1' },
  { code: 'A25', name: '金色', hex: '#FFD67D', group: '1' },
  { code: 'A26', name: '亮金黄', hex: '#FFC830', group: '1' },
  // B 系列
  { code: 'B1', name: '黄绿', hex: '#E6EE31', group: '2' },
  { code: 'B2', name: '鲜绿', hex: '#63F347', group: '2' },
  { code: 'B3', name: '浅绿', hex: '#9EF780', group: '2' },
  { code: 'B4', name: '青翠', hex: '#5DE035', group: '2' },
  { code: 'B5', name: '草绿', hex: '#35E352', group: '2' },
  { code: 'B6', name: '青绿', hex: '#65E2A6', group: '2' },
  { code: 'B7', name: '青苹果绿', hex: '#3DAF80', group: '2' },
  { code: 'B8', name: '深绿', hex: '#1C9C4F', group: '2' },
  { code: 'B9', name: '墨绿', hex: '#27523A', group: '2' },
  { code: 'B10', name: '青草绿', hex: '#95D3C2', group: '2' },
  { code: 'B11', name: '森林绿', hex: '#5D722A', group: '2' },
  { code: 'B12', name: '墨绿', hex: '#166F41', group: '2' },
  { code: 'B13', name: '亮绿', hex: '#CAEB7B', group: '2' },
  { code: 'B14', name: '浅绿', hex: '#ADE946', group: '2' },
  { code: 'B15', name: '深绿', hex: '#2E5132', group: '2' },
  { code: 'B16', name: '淡绿', hex: '#C5ED9C', group: '2' },
  { code: 'B17', name: '青绿', hex: '#9BB13A', group: '2' },
  { code: 'B18', name: '黄绿', hex: '#E6EE49', group: '2' },
  { code: 'B19', name: '青蓝', hex: '#24B88C', group: '2' },
  { code: 'D19', name: '粉紫', hex: '#D8C3D7', group: '4' },
  { code: 'D20', name: '紫红', hex: '#9C32B2', group: '4' },
  { code: 'D21', name: '深紫', hex: '#9A009B', group: '4' },
  { code: 'D22', name: '深蓝', hex: '#333A95', group: '4' },
  { code: 'D23', name: '浅紫', hex: '#EBDAFC', group: '4' },
  { code: 'D24', name: '蓝紫', hex: '#7786E5', group: '4' },
  { code: 'D25', name: '深蓝', hex: '#494FC7', group: '4' },
  { code: 'D26', name: '粉紫', hex: '#DFC2F8', group: '4' },
  // E 系列
  { code: 'E1', name: '粉橙', hex: '#FDD3CC', group: '5' },
  { code: 'E2', name: '粉紫', hex: '#FEC0DF', group: '5' },
  { code: 'E3', name: '亮粉', hex: '#FFB7E7', group: '5' },
  { code: 'E4', name: '玫粉', hex: '#E8649E', group: '5' },
  { code: 'E5', name: '玫红', hex: '#F551A2', group: '5' },
  { code: 'E6', name: '深粉', hex: '#F13D74', group: '5' },
  { code: 'E7', name: '紫粉', hex: '#C63478', group: '5' },
  { code: 'E8', name: '浅粉', hex: '#FFDBE9', group: '5' },
  { code: 'E9', name: '亮紫', hex: '#E970CC', group: '5' },
  { code: 'E10', name: '玫紫', hex: '#D33793', group: '5' },
  { code: 'E11', name: '粉橙', hex: '#FCDDD2', group: '5' },
  { code: 'E12', name: '亮粉', hex: '#F78FC3', group: '5' },
  { code: 'E13', name: '深紫', hex: '#B5006D', group: '5' },
  { code: 'E14', name: '浅橙', hex: '#FFD1BA', group: '5' },
  { code: 'E15', name: '粉橙', hex: '#F8C7C9', group: '5' },
  { code: 'E16', name: '乳粉', hex: '#FFF3EB', group: '5' },
  { code: 'E17', name: '淡粉', hex: '#FFE2EA', group: '5' },
  { code: 'E18', name: '粉红', hex: '#FFC7DB', group: '5' },
  { code: 'E19', name: '粉紫', hex: '#FEBAD5', group: '5' },
  { code: 'E20', name: '灰粉', hex: '#D8C7D1', group: '5' },
  { code: 'E21', name: '灰粉', hex: '#BD9DA1', group: '5' },
  { code: 'E22', name: '紫灰', hex: '#B785A1', group: '5' },
  { code: 'E23', name: '灰紫', hex: '#937A8D', group: '5' },
  { code: 'E24', name: '亮紫', hex: '#E1BCE8', group: '5' },
  // F 系列
  { code: 'F1', name: '橙红', hex: '#FD957B', group: '6' },
  { code: 'F2', name: '正红', hex: '#FC3D46', group: '6' },
  { code: 'F3', name: '鲜红', hex: '#F74941', group: '6' },
  { code: 'F4', name: '亮红', hex: '#FC283C', group: '6' },
  { code: 'F5', name: '深红', hex: '#E7002F', group: '6' },
  { code: 'F6', name: '砖红', hex: '#943630', group: '6' },
  { code: 'F7', name: '暗红', hex: '#971937', group: '6' },
  { code: 'F8', name: '酒红', hex: '#BC0028', group: '6' },
  { code: 'F9', name: '玫红', hex: '#E2677A', group: '6' },
  { code: 'F10', name: '棕红', hex: '#8A4526', group: '6' },
  { code: 'F11', name: '深棕', hex: '#5A2121', group: '6' },
  { code: 'F12', name: '亮红', hex: '#FD4E6A', group: '6' },
  { code: 'F13', name: '橙红', hex: '#F35744', group: '6' },
  { code: 'F14', name: '粉红', hex: '#FFA9AD', group: '6' },
  { code: 'F15', name: '正红', hex: '#D30022', group: '6' },
  { code: 'F16', name: '粉橙', hex: '#FEC2A6', group: '6' },
  { code: 'F17', name: '橙', hex: '#E69C79', group: '6' },
  { code: 'F18', name: '棕', hex: '#D37C46', group: '6' },
  { code: 'F19', name: '暗红', hex: '#C1444A', group: '6' },
  { code: 'F20', name: '灰红', hex: '#CD9391', group: '6' },
  { code: 'F21', name: '粉红', hex: '#F7B4C6', group: '6' },
  { code: 'F22', name: '粉橙', hex: '#FDC0D0', group: '6' },
  { code: 'F23', name: '橙红', hex: '#F67E66', group: '6' },
  { code: 'F24', name: '粉红', hex: '#E698AA', group: '6' },
  { code: 'F25', name: '红', hex: '#E54B4F', group: '6' },
  // G 系列
  { code: 'G1', name: '奶橙', hex: '#FFE2CE', group: '7' },
  { code: 'G2', name: '粉橙', hex: '#FFC4AA', group: '7' },
  { code: 'G3', name: '浅橙', hex: '#F4C3A5', group: '7' },
  { code: 'G4', name: '橙', hex: '#E1B383', group: '7' },
  { code: 'G5', name: '亮橙', hex: '#EDB045', group: '7' },
  { code: 'G6', name: '橘黄', hex: '#E99C17', group: '7' },
  { code: 'G7', name: '棕橙', hex: '#9D5B3E', group: '7' },
  { code: 'G8', name: '棕红', hex: '#753832', group: '7' },
  { code: 'G9', name: '浅棕', hex: '#E6B483', group: '7' },
  { code: 'G10', name: '橙', hex: '#D98C39', group: '7' },
  { code: 'G11', name: '米棕', hex: '#E0C593', group: '7' },
  { code: 'G12', name: '亮橙', hex: '#FFC890', group: '7' },
  { code: 'G13', name: '棕', hex: '#B7714A', group: '7' },
  { code: 'G14', name: '深棕', hex: '#8D614C', group: '7' },
  { code: 'G15', name: '奶黄', hex: '#FCF9E0', group: '7' },
  { code: 'G16', name: '米黄', hex: '#F2D9BA', group: '7' },
  { code: 'G17', name: '棕', hex: '#78524B', group: '7' },
  { code: 'G18', name: '浅奶橙', hex: '#FFE4CC', group: '7' },
  { code: 'G19', name: '橙', hex: '#E07935', group: '7' },
  { code: 'G20', name: '深棕', hex: '#A94023', group: '7' },
  { code: 'G21', name: '棕', hex: '#B88558', group: '7' },
  // H 系列
  { code: 'H1', name: '极浅紫白', hex: '#FDFBFF', group: 'H' },
  { code: 'H2', name: '极浅蓝白', hex: '#FEFFFF', group: 'H' },
  { code: 'H3', name: '浅灰紫', hex: '#B6B1BA', group: 'H' },
  { code: 'H4', name: '中灰', hex: '#89858C', group: 'H' },
  { code: 'H5', name: '深灰', hex: '#48464E', group: 'H' },
  { code: 'H6', name: '暗灰', hex: '#2F2B2F', group: 'H' },
  { code: 'H7', name: '纯黑', hex: '#000000', group: 'H' },
  { code: 'H8', name: '浅粉灰', hex: '#E7D6DB', group: 'H' },
  { code: 'H9', name: '浅灰白', hex: '#EDEDED', group: 'H' },
  { code: 'H10', name: '米白', hex: '#EEE9EA', group: 'H' },
  { code: 'H11', name: '浅紫灰', hex: '#CECDD5', group: 'H' },
  { code: 'H12', name: '象牙白', hex: '#FFF5ED', group: 'H' },
  { code: 'H13', name: '米黄白', hex: '#F5ECD2', group: 'H' },
  { code: 'H14', name: '浅青灰', hex: '#CFD7D3', group: 'H' },
  { code: 'H15', name: '青灰', hex: '#98A6A8', group: 'H' },
  { code: 'H16', name: '深黑褐', hex: '#1D1414', group: 'H' },
  { code: 'H17', name: '粉白', hex: '#F1EDED', group: 'H' },
  { code: 'H18', name: '乳白', hex: '#FFFDF0', group: 'H' },
  { code: 'H19', name: '浅米白', hex: '#F6EFE2', group: 'H' },
  { code: 'H20', name: '蓝灰', hex: '#949FA3', group: 'H' },
  { code: 'H21', name: '淡黄白', hex: '#FFFBE1', group: 'H' },
  { code: 'H22', name: '淡紫灰', hex: '#CACAD4', group: 'H' },
  { code: 'H23', name: '橄榄灰', hex: '#9A9D94', group: 'H' },
  // M 系列
  { code: 'M1', name: '浅青灰绿', hex: '#BCC6B8', group: 'M' },
  { code: 'M2', name: '灰绿', hex: '#8AA386', group: 'M' },
  { code: 'M3', name: '青灰', hex: '#697D80', group: 'M' },
  { code: 'M4', name: '浅米灰', hex: '#E3D2BC', group: 'M' },
  { code: 'M5', name: '米灰', hex: '#D0CCAA', group: 'M' },
  { code: 'M6', name: '卡其灰', hex: '#B0A782', group: 'M' },
  { code: 'M7', name: '灰褐', hex: '#B4A497', group: 'M' },
  { code: 'M8', name: '玫瑰灰', hex: '#B38281', group: 'M' },
  { code: 'M9', name: '土黄灰', hex: '#A58767', group: 'M' },
  { code: 'M10', name: '浅紫灰', hex: '#C5B2BC', group: 'M' },
  { code: 'M11', name: '紫灰', hex: '#9F7594', group: 'M' },
  { code: 'M12', name: '深褐灰', hex: '#644749', group: 'M' },
  { code: 'M13', name: '橙褐', hex: '#D19066', group: 'M' },
  { code: 'M14', name: '砖红褐', hex: '#C77362', group: 'M' },
  { code: 'M15', name: '深青灰', hex: '#757D78', group: 'M' },
  // P 系列
  { code: 'P1', name: '珍珠白', hex: '#FCF7F8', group: 'P' },
  { code: 'P2', name: '银灰', hex: '#B0A9AC', group: 'P' },
  { code: 'P3', name: '薄荷绿', hex: '#AFDCAB', group: 'P' },
  { code: 'P4', name: '珊瑚粉', hex: '#FEA49F', group: 'P' },
  { code: 'P5', name: '南瓜橙', hex: '#EE8C3E', group: 'P' },
  { code: 'P6', name: '青绿', hex: '#5FD0A7', group: 'P' },
  { code: 'P7', name: '杏橙', hex: '#EB9270', group: 'P' },
  { code: 'P8', name: '柠檬黄', hex: '#F0D958', group: 'P' },
  { code: 'P9', name: '浅灰', hex: '#D9D9D9', group: 'P' },
  { code: 'P10', name: '淡紫', hex: '#D9C7EA', group: 'P' },
  { code: 'P11', name: '奶黄', hex: '#F3ECC9', group: 'P' },
  { code: 'P12', name: '冰蓝', hex: '#E6EEF2', group: 'P' },
  { code: 'P13', name: '天蓝', hex: '#AACBEF', group: 'P' },
  { code: 'P14', name: '深青', hex: '#337680', group: 'P' },
  { code: 'P15', name: '墨绿', hex: '#668575', group: 'P' },
  { code: 'P16', name: '明黄', hex: '#FEBF45', group: 'P' },
  { code: 'P17', name: '橙黄', hex: '#FEA324', group: 'P' },
  { code: 'P18', name: '蜜桃粉', hex: '#FEB89F', group: 'P' },
  { code: 'P19', name: '象牙黄', hex: '#FFFEEC', group: 'P' },
  { code: 'P20', name: '樱花粉', hex: '#FEBECF', group: 'P' },
  { code: 'P21', name: '浅玫粉', hex: '#ECBEBF', group: 'P' },
  { code: 'P22', name: '杏粉', hex: '#E4A89F', group: 'P' },
  { code: 'P23', name: '豆沙红', hex: '#A56268', group: 'P' },
  // Q 系列
  { code: 'Q1', name: '亮粉紫', hex: '#F2A5E8', group: 'Q' },
  { code: 'Q2', name: '嫩黄绿', hex: '#E9EC91', group: 'Q' },
  { code: 'Q3', name: '荧光黄', hex: '#FFFF00', group: 'Q' },
  { code: 'Q4', name: '荧光粉', hex: '#FFEBFA', group: 'Q' },
  { code: 'Q5', name: '荧光蓝', hex: '#76CEDE', group: 'Q' },
  // R 系列
  { code: 'R1', name: '正红', hex: '#D50D21', group: 'R' },
  { code: 'R2', name: '玫红', hex: '#F92F83', group: 'R' },
  { code: 'R3', name: '橙红', hex: '#FD8324', group: 'R' },
  { code: 'R4', name: '柠檬黄', hex: '#F8EC31', group: 'R' },
  { code: 'R5', name: '翠绿', hex: '#35C75B', group: 'R' },
  { code: 'R6', name: '孔雀蓝', hex: '#238891', group: 'R' },
  { code: 'R7', name: '湖蓝', hex: '#19779D', group: 'R' },
  { code: 'R8', name: '宝蓝', hex: '#1A60C3', group: 'R' },
  { code: 'R9', name: '紫罗兰', hex: '#9A56B4', group: 'R' },
  { code: 'R10', name: '金黄', hex: '#FFDB4C', group: 'R' },
  { code: 'R11', name: '淡粉白', hex: '#FFEBFA', group: 'R' },
  { code: 'R12', name: '浅灰米', hex: '#D8D5CE', group: 'R' },
  { code: 'R13', name: '深灰褐', hex: '#55514C', group: 'R' },
  { code: 'R14', name: '薄荷青', hex: '#9FE4DF', group: 'R' },
  { code: 'R15', name: '浅天蓝', hex: '#77CEE9', group: 'R' },
  { code: 'R16', name: '青绿蓝', hex: '#3ECFCA', group: 'R' },
  { code: 'R17', name: '深青绿', hex: '#4A867A', group: 'R' },
  { code: 'R18', name: '嫩绿', hex: '#7FCD9D', group: 'R' },
  { code: 'R19', name: '青柠绿', hex: '#CDE55D', group: 'R' },
  { code: 'R20', name: '浅驼色', hex: '#E8C7B4', group: 'R' },
  { code: 'R21', name: '驼色', hex: '#AD6F3C', group: 'R' },
  { code: 'R22', name: '深褐', hex: '#6C372F', group: 'R' },
  { code: 'R23', name: '浅橙黄', hex: '#FEB872', group: 'R' },
  { code: 'R24', name: '浅粉', hex: '#F3C1C0', group: 'R' },
  { code: 'R25', name: '砖红', hex: '#C9675E', group: 'R' },
  { code: 'R26', name: '藕粉', hex: '#D293BE', group: 'R' },
  { code: 'R27', name: '粉玫', hex: '#EA8CB1', group: 'R' },
  { code: 'R28', name: '浅紫', hex: '#9C87D6', group: 'R' },
  // T 系列
  { code: 'T1', name: '纯白', hex: '#FFFFFF', group: 'T' },
  // Y 系列
  { code: 'Y1', name: '荧光玫红', hex: '#FD6FB4', group: 'Y' },
  { code: 'Y2', name: '荧光橙', hex: '#FEB481', group: 'Y' },
  { code: 'Y3', name: '荧光绿', hex: '#D7FAA0', group: 'Y' },
  { code: 'Y4', name: '荧光天蓝', hex: '#8BDBFA', group: 'Y' },
  { code: 'Y5', name: '荧光紫', hex: '#E987EA', group: 'Y' },
  // ZG 系列
  { code: 'ZG1', name: '浅豆沙粉', hex: '#DAABB3', group: 'ZG' },
  { code: 'ZG2', name: '浅驼灰', hex: '#D6AA87', group: 'ZG' },
  { code: 'ZG3', name: '浅卡其', hex: '#C1BD8D', group: 'ZG' },
  { code: 'ZG4', name: '浅紫灰', hex: '#96869F', group: 'ZG' },
  { code: 'ZG5', name: '浅蓝灰', hex: '#8490A6', group: 'ZG' },
  { code: 'ZG6', name: '浅蓝', hex: '#94BFE2', group: 'ZG' },
  { code: 'ZG7', name: '浅粉紫', hex: '#E2A9D2', group: 'ZG' },
  { code: 'ZG8', name: '浅紫', hex: '#AB91C0', group: 'ZG' }
];

// 套装颜色数组
const colorsArray = {
  '1': ['B3', 'C3', 'D9', 'E2', 'G1', 'A4', 'B5', 'C5', 'D6', 'E4', 'G5', 'A6', 'B8', 'C8', 'D7', 'F5', 'G7', 'A7', 'H1', 'H2', 'H3', 'H4', 'H5', 'H7'],
  '2': ['C2', 'C13', 'D19', 'E8', 'A13', 'A11', 'C10', 'C6', 'D18', 'E3', 'A10', 'G9', 'C11', 'C7', 'D21', 'D13', 'F13', 'G13', 'B12', 'D3', 'D15', 'E7', 'F8', 'G8'],
  '3': ['A3', 'B20', 'D16', 'D8', 'E1', 'G2', 'B18', 'B10', 'D11', 'D12', 'E12', 'G3', 'B14', 'B19', 'D2', 'D20', 'E5', 'F10', 'B17', 'B7', 'C16', 'D14', 'E13', 'F7'],
  '4': ['E11', 'E14', 'F1', 'A14', 'M6', 'M5', 'E15', 'F14', 'F9', 'F2', 'G14', 'M9', 'E9', 'E6', 'F12', 'F3', 'F11', 'M12', 'D5', 'E10', 'F4', 'F6', 'G17', 'H6'],
  '5': ['A15', 'A5', 'A8', 'A12', 'A9', 'G6', 'A1', 'B13', 'B1', 'B2', 'B4', 'B11', 'H12', 'C1', 'B16', 'B6', 'C15', 'B15', 'C14', 'D17', 'D1', 'C4', 'C17', 'C9'],
  '6': ['H8', 'G15', 'A2', 'H13', 'G16', 'H9', 'H10', 'M1', 'G11', 'G4', 'M4', 'H14', 'M10', 'M2', 'G12', 'M13', 'M7', 'H11', 'M11', 'M3', 'G10', 'M14', 'M8', 'M15'],
  '9': ['H17', 'H18', 'H19', 'E16', 'F16', 'F17', 'D23', 'E24', 'E19', 'E18', 'E17', 'E20', 'B24', 'A16', 'A17', 'A18', 'F24', 'F23', 'A24', 'A22', 'A21', 'F21', 'F22', 'A19'],
  '10': ['A26', 'A25', 'A20', 'A23', 'G18', 'H21', 'B26', 'B32', 'B31', 'B30', 'B27', 'B29', 'C22', 'C23', 'C24', 'B28', 'C25', 'C27', 'H15', 'H20', 'H23', 'H22', 'C28', 'C21'],
  '11': ['F15', 'F19', 'G20', 'E21', 'E22', 'D26', 'F25', 'F20', 'G19', 'F18', 'G21', 'E23', 'D25', 'D22', 'D24', 'C20', 'B21', 'B25', 'H16', 'B23', 'C18', 'B22', 'C19', 'C26'],
  'A': ['B10', 'C2', 'C3', 'C13', 'D16', 'D17', 'B6', 'C4', 'C10', 'C17', 'D1', 'D11', 'C15', 'C11', 'C5', 'C6', 'C7', 'D2', 'B19', 'B7', 'C8', 'C9', 'D3', 'C16'],
  'B': ['E12', 'E2', 'E8', 'D19', 'D8', 'D9', 'E6', 'E4', 'E3', 'E9', 'D12', 'D6', 'E5', 'E10', 'D5', 'D13', 'D20', 'D18', 'E7', 'E13', 'D21', 'D14', 'D7', 'D15'],
  'C': ['C14', 'B20', 'C1', 'B18', 'M5', 'M6', 'B3', 'B16', 'B13', 'B1', 'G13', 'F10', 'B5', 'B4', 'B2', 'B14', 'G7', 'F11', 'B15', 'B12', 'B8', 'B17', 'B11', 'G8'],
  'D': ['A15', 'A3', 'A11', 'A9', 'F14', 'F12', 'A4', 'A13', 'A6', 'F1', 'F2', 'F3', 'A5', 'A10', 'A7', 'F13', 'F9', 'F6', 'A8', 'A14', 'F4', 'F5', 'F8', 'F7'],
  'E': ['E15', 'E1', 'E14', 'E11', 'H2', 'H1', 'A12', 'G3', 'G2', 'G1', 'A1', 'H12', 'G6', 'G5', 'G9', 'M9', 'H3', 'H4', 'G14', 'M12', 'G17', 'H5', 'H6', 'H7']
};

// 官方套装配置
const ARTKAL_OFFICIAL_SETS = {
  set24: {
    name: '24色套装',
    colors: colorsArray['1']
  },
  set48: {
    name: '48色套装',
    colors: [...colorsArray['1'], ...colorsArray['2']]
  },
  set72: {
    name: '72色套装',
    colors: [...colorsArray['1'], ...colorsArray['2'], ...colorsArray['3']]
  },
  set96: {
    name: '96色套装',
    colors: [...colorsArray['1'], ...colorsArray['2'], ...colorsArray['3'], ...colorsArray['4']]
  },
  set120: {
    name: '120色套装',
    colors: [...colorsArray['A'], ...colorsArray['B'], ...colorsArray['C'], ...colorsArray['D'], ...colorsArray['E']]
  },
  set144: {
    name: '144色套装',
    colors: [...colorsArray['A'], ...colorsArray['B'], ...colorsArray['C'], ...colorsArray['D'], ...colorsArray['E'], ...colorsArray['6']]
  },
  set216: {
    name: '216色套装',
    colors: [...colorsArray['A'], ...colorsArray['B'], ...colorsArray['C'], ...colorsArray['D'], ...colorsArray['E'], ...colorsArray['6'], ...colorsArray['9'], ...colorsArray['10'], ...colorsArray['11']]
  },
  set221: {
    name: '221色套装',
    colors: [
      ...colorsArray['A'],
      ...colorsArray['B'],
      ...colorsArray['C'],
      ...colorsArray['D'],
      ...colorsArray['E'],
      ...colorsArray['6'],
      ...colorsArray['9'],
      ...colorsArray['10'],
      ...colorsArray['11'],
      'C29', 'D10', 'B9', 'C12', 'D4'
    ]
  }
};

// 预设列表（用于UI显示）
const ARTKAL_PRESETS = [
  { key: 'set24', name: '24色套装', count: 24 },
  { key: 'set48', name: '48色套装', count: 48 },
  { key: 'set72', name: '72色套装', count: 72 },
  { key: 'set96', name: '96色套装', count: 96 },
  { key: 'set120', name: '120色套装', count: 120 },
  { key: 'set144', name: '144色套装', count: 144 },
  { key: 'set216', name: '216色套装', count: 216 },
  { key: 'set221', name: '221色套装', count: 221 }
];

// 添加RGB值到每个颜色对象
ARTKAL_COLORS_FULL.forEach(color => {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(color.hex);
  if (result) {
    color.r = parseInt(result[1], 16);
    color.g = parseInt(result[2], 16);
    color.b = parseInt(result[3], 16);
  }
});

// 根据色号查找颜色
function getColorByCode(code) {
  return ARTKAL_COLORS_FULL.find(c => c.code === code);
}

// 找到最接近的颜色（欧几里得距离）
function findClosestColor(r, g, b, colorCodes) {
  const colors = colorCodes.map(code => getColorByCode(code)).filter(c => c);
  if (colors.length === 0) return null;
  
  let minDistance = Infinity;
  let closestColor = colors[0];
  
  for (const color of colors) {
    const distance = Math.sqrt(
      Math.pow(r - color.r, 2) +
      Math.pow(g - color.g, 2) +
      Math.pow(b - color.b, 2)
    );
    
    if (distance < minDistance) {
      minDistance = distance;
      closestColor = color;
    }
  }
  
  return closestColor;
}

// 导出
export { 
  ARTKAL_COLORS_FULL, 
  ARTKAL_OFFICIAL_SETS, 
  ARTKAL_PRESETS, 
  getColorByCode,
  findClosestColor
};