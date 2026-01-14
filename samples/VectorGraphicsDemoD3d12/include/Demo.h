/*
 Demo base class for VectorGraphics D3D12 demo framework

 Each demo showcases different features of the vg::CanvasD3d12 API.
*/
#pragma once

#include "cinder/app/App.h"
#include "cinder/CanvasUi.h"
#include "cinder/vg/CanvasD3d12.h"

namespace vg = cinder::vg;

class Demo {
public:
    virtual ~Demo() = default;

    // Called once when demo is first selected
    virtual void setup( vg::CanvasD3d12Ref canvas ) {}

    // Called when demo is selected/deselected
    virtual void onActivate() {}
    virtual void onDeactivate() {}

    // Called each frame
    virtual void update( double deltaTime ) {}

    // Draw vector content (canvas transform already applied)
    virtual void draw( vg::CanvasD3d12Ref canvas ) = 0;

    // Draw ImGui controls for this demo
    virtual void drawUI() {}

    // Set CanvasUi reference for coordinate transformation
    void setCanvasUi( ci::CanvasUi* ui ) { mCanvasUi = ui; }

    // Input handlers - return true if handled
    virtual bool onMouseDown( ci::app::MouseEvent event ) { return false; }
    virtual bool onMouseDrag( ci::app::MouseEvent event ) { return false; }
    virtual bool onMouseUp( ci::app::MouseEvent event ) { return false; }
    virtual bool onMouseMove( ci::app::MouseEvent event ) { return false; }

    // Demo info
    virtual std::string getName() const = 0;
    virtual std::string getDescription() const { return ""; }

    // Content bounds for CanvasUi
    virtual ci::Rectf getContentBounds() const { return ci::Rectf( 0, 0, 800, 600 ); }

protected:
    vg::CanvasD3d12Ref mCanvas;
    ci::CanvasUi* mCanvasUi = nullptr;
};

using DemoRef = std::shared_ptr<Demo>;
