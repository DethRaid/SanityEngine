import "imgui" for ImGui

class EnvironmentObjectEditor {
    construct new() {}
    
    draw() {
        ImGui.Begin("Environment Object")
        ImGui.Label("This is where the object stuff will go")
        ImGui.End()
    }
}
