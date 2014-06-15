You’ll need to have the logic software installed, and typically the same or higher version as the SDK you’re using. 
 
--Launch the Logic software
--Options->Preferences
--Under “(For Developers) Search this path for Analyzer Plugins” browse to the Debug (or later, Release) folder in your project.
--Click “Save” and then close the Logic software. 


We need to tell Visual Studio to use the Logic software in its debug session.

--In Visual Studio, right click in the Project item (under Solution Explorer) and select Properties.  
--Click the Debugging Item
--Under Command click in the field, click the down arrow that appears, and select “Browse…”
--Navigate to the Logic.exe program.  This is typically located at “C:\Program Files\Saleae LLC”  (Note that the older 1.0.33 version, if it is installed, is located in the folder “C:\Program Files\Logic” – be sure not to accidently use this version).
-- Click OK

