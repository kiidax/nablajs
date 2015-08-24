var d = new Date();
print(typeof d.getTime());
print(d.getTime() > 1400000000000);
print(d.getTime() - d.getTime());
d = new Date(12345.3);
print(d.getTime());
