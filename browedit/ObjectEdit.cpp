#include "BrowEdit.h"

#include <blib/wm/WM.h>

#include <BroLib/Map.h>
#include <BroLib/Gnd.h>

#include "actions/SelectObjectAction.h"
#include "actions/ObjectEditAction.h"
#include "actions/GroupAction.h"

#include "windows/ModelPropertiesWindow.h"

void BrowEdit::objectEditUpdate()
{
	if (!wm->inWindow(mouseState.x, mouseState.y))
	{
		if (newModel)
		{
			newModel->position = glm::vec3(mapRenderer.mouse3d.x - map->getGnd()->width * 5, -mapRenderer.mouse3d.y, -mapRenderer.mouse3d.z + (10 + 5 * map->getGnd()->height));;
			newModel->matrixCached = false;
			if (mouseState.leftButton && !lastMouseState.leftButton)
				newModel = NULL;
		}

		else if (mouseState.leftButton && !lastMouseState.leftButton || mouseState.rightButton && !lastMouseState.rightButton)
		{//down
			startMouseState = mouseState;
			mouse3dstart = mapRenderer.mouse3d;

			selectObjectAction = new SelectObjectAction(map->getRsw());
			glm::vec3 center;
			int selectCount = 0;
			for (size_t i = 0; i < map->getRsw()->objects.size(); i++)
			{
				if (!map->getRsw()->objects[i]->selected)
					continue;
				selectCount++;
				center += glm::vec3(5 * map->getGnd()->width + map->getRsw()->objects[i]->position.x, -map->getRsw()->objects[i]->position.y, 10 + 5 * map->getGnd()->height - map->getRsw()->objects[i]->position.z);
			}

			if (selectCount > 0)
			{
				center /= selectCount;
				objectTranslateDirection = TranslatorTool::Axis::NONE;
				objectRotateDirection = RotatorTool::Axis::NONE;

				if (objectEditModeTool == ObjectEditModeTool::Translate)
					objectTranslateDirection = translatorTool.selectedAxis(mapRenderer.mouseRay, center);
				if (objectEditModeTool == ObjectEditModeTool::Rotate)
					objectRotateDirection = rotatorTool.selectedAxis(mapRenderer.mouseRay, center);
				if (objectEditModeTool == ObjectEditModeTool::Scale)
					objectScaleDirection = scaleTool.selectedAxis(mapRenderer.mouseRay, center);


				for (size_t i = 0; i < map->getRsw()->objects.size(); i++)
					if (map->getRsw()->objects[i]->selected)
						objectEditActions.push_back(new ObjectEditAction(map->getRsw()->objects[i], i));
			}




		}
		else if (!mouseState.leftButton && lastMouseState.leftButton)
		{//left up
			if (objectTranslateDirection != TranslatorTool::Axis::NONE || objectRotateDirection != RotatorTool::Axis::NONE || objectScaleDirection != ScaleTool::Axis::NONE)
			{
				GroupAction* action = new GroupAction();
				for (ObjectEditAction* a : objectEditActions)
					action->add(a);
				perform(action);
				objectEditActions.clear();
			}
			else if (abs(startMouseState.x - lastMouseState.x) < 2 && abs(startMouseState.y - lastMouseState.y) < 2)
			{ //click
				if (!wm->inWindow(mouseState.x, mouseState.y))
				{
					for (size_t i = 0; i < map->getRsw()->objects.size(); i++)
					{
						Rsw::Object* o = map->getRsw()->objects[i];
						glm::vec2 pos = glm::vec2(map->getGnd()->width * 5 + o->position.x, 10 + 5 * map->getGnd()->height - o->position.z);
						float dist = glm::length(pos - glm::vec2(mapRenderer.mouse3d.x, mapRenderer.mouse3d.z));
						o->selected = o->collides(mapRenderer.mouseRay);
						if (o->selected && mouseState.clickcount == 2)
						{
							if (o->type == Rsw::Object::Type::Model)
								new ModelPropertiesWindow((Rsw::Model*)o, resourceManager);
						}
					}
				}
			}
			else
			{
				for (size_t i = 0; i < map->getRsw()->objects.size(); i++)
				{
					Rsw::Object* o = map->getRsw()->objects[i];
					glm::vec2 tl = glm::vec2(glm::min(mouse3dstart.x, mapRenderer.mouse3d.x), glm::min(mouse3dstart.z, mapRenderer.mouse3d.z));
					glm::vec2 br = glm::vec2(glm::max(mouse3dstart.x, mapRenderer.mouse3d.x), glm::max(mouse3dstart.z, mapRenderer.mouse3d.z));
					glm::vec2 pos = glm::vec2(map->getGnd()->width * 5 + o->position.x, 10 + 5 * map->getGnd()->height - o->position.z);

					o->selected = pos.x > tl.x && pos.x < br.x && pos.y > tl.y && pos.y < br.y;
				}
			}
			selectObjectAction->finish(map->getRsw());
			if (selectObjectAction->hasDifference())
				perform(selectObjectAction);
			else
			{
				delete selectObjectAction;
				selectObjectAction = NULL;
			}
		}
		else if (mouseState.leftButton && lastMouseState.leftButton)
		{ // dragging				
			for (size_t i = 0; i < map->getRsw()->objects.size(); i++)
			{
				Rsw::Object* o = map->getRsw()->objects[i];
				if (o->selected)
				{
					if (objectTranslateDirection != TranslatorTool::Axis::NONE)
					{
						if (((int)objectTranslateDirection & (int)TranslatorTool::Axis::X) != 0)
							o->position.x -= lastmouse3d.x - mapRenderer.mouse3d.x;
						if (((int)objectTranslateDirection & (int)TranslatorTool::Axis::Y) != 0)
							o->position.y += (mouseState.y - lastMouseState.y) / 5.0f;
						if (((int)objectTranslateDirection & (int)TranslatorTool::Axis::Z) != 0)
							o->position.z += lastmouse3d.z - mapRenderer.mouse3d.z;
					}
					if (objectRotateDirection != RotatorTool::Axis::NONE)
					{
						if (objectRotateDirection == RotatorTool::Axis::X)
							o->rotation.x -= mouseState.x - lastMouseState.x;
						if (objectRotateDirection == RotatorTool::Axis::Y)
							o->rotation.y -= mouseState.x - lastMouseState.x;
						if (objectRotateDirection == RotatorTool::Axis::Z)
							o->rotation.z -= mouseState.x - lastMouseState.x;
					}
					if (objectScaleDirection != ScaleTool::Axis::NONE)
					{
						if (objectScaleDirection == ScaleTool::Axis::X)
							o->scale.x *= 1 - (mouseState.x - lastMouseState.x + mouseState.y - lastMouseState.y) * 0.01f;
						if (objectScaleDirection == ScaleTool::Axis::Y)
							o->scale.y *= 1 - (mouseState.x - lastMouseState.x + mouseState.y - lastMouseState.y) * 0.01f;
						if (objectScaleDirection == ScaleTool::Axis::Z)
							o->scale.z *= 1 - (mouseState.x - lastMouseState.x + mouseState.y - lastMouseState.y) * 0.01f;
						if (objectScaleDirection == ScaleTool::Axis::ALL)
							o->scale *= 1 - (mouseState.x - lastMouseState.x + mouseState.y - lastMouseState.y) * 0.01f;
					}

					((Rsw::Model*)o)->matrixCached = false;
				}
			}
		}


		if (!mouseState.rightButton && lastMouseState.rightButton)
		{
			for (size_t i = 0; i < map->getRsw()->objects.size(); i++)
				map->getRsw()->objects[i]->selected = false;
		}
	}





	if (true /* wm keyfocus */)
	{
		if (keyState.isPressed(blib::Key::DEL) && !lastKeyState.isPressed(blib::Key::DEL))
		{
			for (int i = 0; i < (int)map->getRsw()->objects.size(); i++)
			{
				if (map->getRsw()->objects[i]->selected)
				{
					map->getRsw()->objects.erase(map->getRsw()->objects.begin() + i);
					i--;
				}
			}
		}
	}
}