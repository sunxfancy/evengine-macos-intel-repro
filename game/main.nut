local frames=0;
local mapModule=null;
local layer=null;
eve_init = function() {
    gfx.setBackgroundColor(0.08,0.09,0.12,1.0);
    mapModule=eve.Map();
    layer=mapModule.newLayer(10,8,28.0,28.0);
    layer.setOrigin(40.0,40.0);
    for(local y=0;y<8;y++)for(local x=0;x<10;x++)layer.setTile(x,y,1);
    layer.setVisible(true);
};
eve_update = function(dt) {
    frames++;
    if(frames==2) print("MAP_CAPTURE_WARM="+gfx.saveFramePng("/tmp/eve-map-warm.png")+"\n");
    if(frames==3) print("MAP_CAPTURE_EARLY="+gfx.saveFramePng("/tmp/eve-map-early.png")+"\n");
    if(frames==20) {
        print("MAP_CAPTURE_FINAL="+gfx.saveFramePng("/tmp/eve-map-final.png")+"\n");
        win.close();
    }
};
eve_render = function() {
    gfx.clear();
    mapModule.render(gfx);
    gfx.drawSolidRect(360.0,40.0,100.0,100.0,1.0,0.0,0.0,1.0);
};
