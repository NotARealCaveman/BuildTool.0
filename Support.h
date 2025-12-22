#pragma once
#include <conio.h> 
#include <unordered_map>

#include <Engine/Builders/DatabaseBuilder.h>
#include <Engine/Modules/EngineModule.h>

namespace ReignEngine
{
	void HideConsoleCursor()
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hConsole == INVALID_HANDLE_VALUE) return;

		CONSOLE_CURSOR_INFO cursorInfo;
		GetConsoleCursorInfo(hConsole, &cursorInfo);  // get current cursor info
		cursorInfo.bVisible = FALSE;                  // hide the cursor
		SetConsoleCursorInfo(hConsole, &cursorInfo);  // apply
	}

	using InputCode = MFint32;
	enum BUILD_KEY_CODES : InputCode
	{
		DEFAULT,
		UP,
		DOWN,
		LEFT,
		RIGHT,
		ESCAPE,
		BACKSPACE,
		DEL,
		RETURN,
	};

	inline BUILD_KEY_CODES GetKey(InputCode& inputCode)
	{
		inputCode = _getch();
		//detect special keys
		if (inputCode == 0x0 || inputCode == 0xE0)
			switch (_getch())
			{
				case 0x48: return BUILD_KEY_CODES::UP;
				case 0x50: return BUILD_KEY_CODES::DOWN;
				case 0x4B: return BUILD_KEY_CODES::LEFT;
				case 0x4D: return BUILD_KEY_CODES::RIGHT;
				case 0x53: return BUILD_KEY_CODES::DEL;
			}

		switch (inputCode)
		{
			case 0x1B: return BUILD_KEY_CODES::ESCAPE;
			case 0x08: return BUILD_KEY_CODES::BACKSPACE;
			case 0x0D: return BUILD_KEY_CODES::RETURN;
		}

		return BUILD_KEY_CODES::DEFAULT;
	}

	std::unordered_map<EntryID, MFstring> symbols;
	std::stack<std::unique_ptr<Form>> formStack;
	const FormKeyMap keyMap
	{
		///UP KEY
		std::make_pair(BUILD_KEY_CODES::UP, [](const InputCode)->void
		{
			std::unique_ptr<Form>& currentForm{ formStack.top() };
			currentForm->bodyLines.at(currentForm->activeLine).isActive = false;
			if (currentForm->activeLine)
				--currentForm->activeLine;
			else
				currentForm->activeLine = currentForm->bodyLines.size() - 1;
			currentForm->bodyLines.at(currentForm->activeLine).isActive = true;
		}),
			///DOWN KEY
			std::make_pair(BUILD_KEY_CODES::DOWN, [](const InputCode)->void
			{
				std::unique_ptr<Form>& currentForm{ formStack.top() };
				currentForm->bodyLines.at(currentForm->activeLine).isActive = false;
				if (currentForm->activeLine == currentForm->bodyLines.size() - 1)
					currentForm->activeLine = 0;
				else
					++currentForm->activeLine;
				currentForm->bodyLines.at(currentForm->activeLine).isActive = true;
			}),
			///RIGHT KEY
			std::make_pair(BUILD_KEY_CODES::RIGHT, [](const InputCode)->void
			{

				std::unique_ptr<Form>& currentForm{ formStack.top() };
				FormLine& currentLine{ currentForm->bodyLines.at(currentForm->activeLine) };
				currentLine.textBuffer.bufferPosition = std::min(currentLine.textBuffer.bufferPosition + 1, currentLine.textBuffer.buffer.size());
			}),
			///LEFT KEY
			std::make_pair(BUILD_KEY_CODES::LEFT, [](const InputCode)->void
			{

				std::unique_ptr<Form>& currentForm{ formStack.top() };
				FormLine& currentLine{ currentForm->bodyLines.at(currentForm->activeLine) };
				if (currentLine.textBuffer.bufferPosition > 0)
					currentLine.textBuffer.bufferPosition -= 1;
			}),
			///RETURN KEY
			std::make_pair(BUILD_KEY_CODES::RETURN, [](const InputCode)->void
			{
				std::unique_ptr<Form>& currentForm{ formStack.top() };
				const FormLine& menuLine{ currentForm->bodyLines.at(currentForm->activeLine) };
				ForwardFunction(menuLine.lineFunction, BUILD_KEY_CODES::RETURN);
			}),
			///ESCAPE KEY
			std::make_pair(BUILD_KEY_CODES::ESCAPE, [](const InputCode)->void
			{
			//get resource out of stack
			const std::unique_ptr<Form> currentForm{ std::move(formStack.top()) };
			formStack.pop();//remove form stack node
			//exit function may push new forms onto stack - cannot pop after
			currentForm->exitFunction(0);//createOnClose bound to form ptr
		}),
			///BACKSPACE KEY
			std::make_pair(BUILD_KEY_CODES::BACKSPACE, [](const InputCode)->void
			{
				std::unique_ptr<Form>& currentForm{ formStack.top() };
				FormLine& currentLine{ currentForm->bodyLines.at(currentForm->activeLine) };
				TextBuffer& textBuffer{ currentLine.textBuffer };
				textBuffer.Backspace();
			}),
			///DELETE KEY
			std::make_pair(BUILD_KEY_CODES::DEL, [](const InputCode)->void
			{
				std::unique_ptr<Form>& currentForm{ formStack.top() };
				FormLine& currentLine{ currentForm->bodyLines.at(currentForm->activeLine) };
				TextBuffer& textBuffer{ currentLine.textBuffer };
				textBuffer.Delete();
			}),
			///ALPHA-NUMERIC KEY
			std::make_pair(BUILD_KEY_CODES::DEFAULT, [](const InputCode keyCode)->void
			{
			//range printable defualt keys to alphanumeric + general symbol 
			if (keyCode < 0x20 || keyCode > 0xE7)
				return;

			std::unique_ptr<Form>& currentForm{ formStack.top() };
			FormLine& currentLine{ currentForm->bodyLines.at(currentForm->activeLine) };
			if (currentLine.isEditable)
				currentLine.textBuffer.Insert(keyCode);
		}),
	};
}