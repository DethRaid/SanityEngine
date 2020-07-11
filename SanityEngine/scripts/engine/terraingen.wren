import "imgui" for ImGui

class EnvironmentObjectEditor {
    construct new() {}
    
    draw() {
        ImGui.Begin("Environment Object", Box.new(true), 0)
        ImGui.Label("This is where the object stuff will go")
        ImGui.End()
    }
}
