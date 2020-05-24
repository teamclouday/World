# Basic Structure  

------

## class Application  
* Should be created in global context  
* Manage variables and provide reference to them  
* Provide interface to manage these variables  
* Store user-defined settings for rendering  

------

## namespace BASE {  

### class Backend  
* Created in Application object  
* Manage low-level variables  
* Setup a Vulkan context for rendering  
* Provide interface for device related information  

### class Renderer  
* Created in Application object  
* Manage variables for rendering  
* Setup a Vulkan render pipeline by the user settings  
* Provide interface for render and loop  

## }  

------

## namespace DATA {  

### class Graph  
* Created in Renderer object  
* Store render resources: buffers, textures, meshes  
* Provide command buffers for Renderer

## }  

------

## namespace UTILS {  

### class Camera  
* Created in Application object  
* Provide camera view for users  

## }

------

## namespace FILES {

This namespace is actually redundant, can be combined to UTILS  

## }  

------

## namespace LOGGING {  

### class Logger  
* Created in Application object  
* Can be disabled in compile time  
* Used everywhere, to store important messages  

## }  