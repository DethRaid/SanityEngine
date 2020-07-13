import "imgui" for ImGui, Box, ImVec2

class EnvironmentObjectEditor {
    construct new() {}
    
    draw() {
        var pos = ImVec2.new(300, 300)
        var pivot = ImVec2.new(0.5, 0.5)
        ImGui.SetNextWindowPos(pos, "Always", pivot)

        ImGui.Begin("Environment Object", Box.new(true), 0)
        ImGui.Text("This is where the object stuff will go")
        ImGui.End()
    }
}
