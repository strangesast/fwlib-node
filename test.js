var {Fwlib} = require('./index.js');

var obj = new Fwlib('localhost', 8193);
obj.connect();

const cncid = obj.rdcncid()
                  .map(d => ('0'.repeat(8) + d.toString(16)).slice(-8))
                  .join('-');
console.log(`machine id: ${cncid}`);

console.log(obj.sysinfo());

console.log('rdaxisdata', obj.rdaxisdata(1, [0, 1, 2]));

obj.disconnect();
