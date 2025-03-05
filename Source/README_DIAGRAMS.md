# Video Manager Pipeline Diagrams

This folder contains PlantUML diagrams representing the architecture of the Video Manager Pipeline system.

## Diagram Files

1. **VideoManagerFlow.puml**: Sequence diagram showing the flow of operations through the pipeline components.
2. **VideoManagerComponent.puml**: Component diagram showing the high-level structure and relationships.
3. **VideoManagerClass.puml**: Class diagram showing the detailed class structure and relationships.

## Viewing the Diagrams

There are several ways to view PlantUML diagrams:

### Online PlantUML Viewer

1. Visit the [PlantUML Online Server](https://www.plantuml.com/plantuml/uml/)
2. Copy and paste the content of the PUML file into the editor
3. The diagram will render automatically

### VS Code Extension

If you're using Visual Studio Code:

1. Install the "PlantUML" extension (by jebbs)
2. Open the .puml file
3. Right-click in the editor and select "Preview Current Diagram"
   - Or use Alt+D to preview

### IntelliJ IDEA Plugin

If you're using IntelliJ IDEA:

1. Install the "PlantUML integration" plugin
2. Open the .puml file
3. The diagram should render in a split view automatically

### Local PlantUML Rendering

If you have Java installed:

1. Download the PlantUML JAR from [PlantUML website](https://plantuml.com/download)
2. Run the following command:
   ```
   java -jar plantuml.jar VideoManagerFlow.puml
   ```
3. This will generate a PNG file of the diagram

## Diagram Overview

### Flow Diagram
The flow diagram shows the sequence of operations from a client request through the pipeline system. It illustrates how requests are processed, validated, and executed.

### Component Diagram
The component diagram shows the major components of the system, including the Main application, PipelineProcess, PipelineManager, PipelineHandler, and StreamDiscoverer. It illustrates how these components interact with each other.

### Class Diagram
The class diagram provides a detailed view of the class structure, including attributes, methods, and relationships between classes.
