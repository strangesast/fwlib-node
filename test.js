var {Fwlib} = require('./index.js');

var obj = new Fwlib('localhost', 8193);
obj.connect();

const cncid = obj.rdcncid()
                  .map(d => ('0'.repeat(8) + d.toString(16)).slice(-8))
                  .join('-');
console.log(`machine id: ${cncid}`);

console.log('sysinfo', obj.sysinfo());

console.log('rdaxisname', obj.rdaxisname());
console.log('rdaxisdata', obj.rdaxisdata(1, [0, 1, 2]));

console.log('exeprgname', obj.exeprgname());
console.log('exeprgname2', `"${obj.exeprgname2()}"`);

const exe_enum = ({run, motion, mstb}) =>
    ['ACTIVE', 'INTERRUPTED', 'STOPPED',
     'READY'][(run == 3 || run == 4) ? 0 : (run == 2 || motion == 2 || mstb != 0) ? 1 : run == 0 ? 2 : 3];
const statinfo = obj.statinfo();
console.log('status', statinfo, exe_enum(statinfo));

console.log('rddynamic2', obj.rddynamic2());

console.log('rdsvmeter', obj.rdsvmeter());

obj.disconnect();
