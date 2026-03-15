// Generate palette based on size
export const generatePalette = (count) => {
  const colors = [];
  
  const base = [
    '#000000', '#ffffff', '#ff0000', '#00ff00', '#0000ff', '#ffff00', '#00ffff', '#ff00ff',
    '#808080', '#c0c0c0', '#800000', '#808000', '#008000', '#800080', '#008080', '#000080',
    '#ffa500', '#a52a2a', '#deb887', '#5f9ea0', '#7fff00', '#d2691e', '#ff7f50', '#6495ed',
    '#dc143c', '#00ced1', '#9400d3', '#ff1493', '#00bfff', '#696969', '#1e90ff', '#b22222'
  ];
  
  for (let i = 0; i < Math.min(count, base.length); i++) {
    colors.push(base[i]);
  }
  
  if (count > base.length) {
    const remaining = count - base.length;
    for (let i = 0; i < remaining; i++) {
      const hue = Math.floor((i / remaining) * 360);
      const sat = 60 + (i % 4) * 10; 
      const light = 40 + (i % 3) * 20;
      colors.push(`hsl(${hue}, ${sat}%, ${light}%)`);
    }
  }
  
  return colors;
};
