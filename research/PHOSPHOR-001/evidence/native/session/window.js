for (const w of workspace.windowList()) {
  if (String(w.resourceClass).toLowerCase() === 'org.phosphor.spike001') {
    workspace.activeWindow = w;
    w.frameGeometry = {x:100, y:80, width:1500, height:1100};
    print('PHOSPHOR_WINDOW ' + JSON.stringify({id:String(w.internalId),pid:w.pid,caption:w.caption,frame:w.frameGeometry,client:w.clientGeometry,buffer:w.bufferGeometry}));
  }
}
