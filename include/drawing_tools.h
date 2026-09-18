enum DrawingToolType { DRAWING_TOOL_PEN, DRAWING_TOOL_LINE, DRAWING_TOOL_FILL };

void initializeDrawingTools(EditorCanvas *_editorCanvas);
void setDrawingTool(enum DrawingToolType);
void drawingToolOnMouseClick(int x, int y);
void drawingToolOnMouseDown(int x, int y);
void drawingToolOnMouseRelease(int x, int y);
