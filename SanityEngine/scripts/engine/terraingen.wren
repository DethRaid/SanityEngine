import "imgui" for ImGui, Box, ImVec2

class EnvironmentObjectEditor {
    construct new() {}
    
    draw() {
        var pos = ImVec2.new(0, 200)
        var pivot = ImVec2.new(0.5, 0.5)
        ImGui.SetNextWindowPos(pos, "Always", pivot)

        ImGui.Begin("Environment Object Editor", Box.new(true), 0)
        ImGui.Text("This is where the object stuff will go")
        ImGui.End()
    }
}
