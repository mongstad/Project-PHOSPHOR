import dbus, dbus.service, dbus.mainloop.glib
from gi.repository import GLib
import argparse,json,tempfile,subprocess,os
p=argparse.ArgumentParser(); p.add_argument('--width',type=int);p.add_argument('--height',type=int);p.add_argument('--x',type=int,default=100);p.add_argument('--y',type=int,default=80)
a=p.parse_args()
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus=dbus.SessionBus(); name=dbus.service.BusName('org.phosphor.TestBridge',bus);loop=GLib.MainLoop();result=[]
class Bridge(dbus.service.Object):
 @dbus.service.method('org.phosphor.TestBridge',in_signature='s',out_signature='')
 def record(self,data):
  result.append(json.loads(data)); loop.quit()
bridge=Bridge(bus,'/Bridge')
action=''
if a.width: action=f'w.frameGeometry = {{x:{a.x},y:{a.y},width:{a.width},height:{a.height}}}; workspace.activeWindow=w;'
js='''const result=[]; for (const w of workspace.windowList()) { if (String(w.resourceClass).toLowerCase().indexOf('phosphor')>=0) { ACTION result.push({id:String(w.internalId),pid:w.pid,caption:w.caption,resourceClass:String(w.resourceClass),frame:w.frameGeometry,client:w.clientGeometry,buffer:w.bufferGeometry}); }} callDBus('org.phosphor.TestBridge','/Bridge','org.phosphor.TestBridge','record',JSON.stringify(result));'''.replace('ACTION',action)
path='/tmp/phosphor-001-native/session/kwin-action.js';open(path,'w').write(js)
manager=dbus.Interface(bus.get_object('org.kde.KWin','/Scripting'),'org.kde.kwin.Scripting')
manager.unloadScript('phosphor-native-bridge')
script=int(bus.call_blocking('org.kde.KWin','/Scripting','org.kde.kwin.Scripting','loadScript','ss',(path,'phosphor-native-bridge')))
dbus.Interface(bus.get_object('org.kde.KWin',f'/Scripting/Script{script}'),'org.kde.kwin.Script').run()
GLib.timeout_add(4000,lambda:(loop.quit(),False)[1]);loop.run();manager.unloadScript('phosphor-native-bridge')
print(json.dumps(result[0] if result else {'error':'timeout'}))
