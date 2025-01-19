[EntityEditorProps(category: "game/", description: "Crews compartments after initialization with their default passenger.")]
class TILW_VehicleCrewComponentClass: ScriptComponentClass
{
}

class TILW_VehicleCrewComponent: ScriptComponent
{
	[Attribute("0", UIWidgets.Auto, desc: "Spawn Default Driver", category: "Crew")]
	protected bool m_spawnPilot;
	[Attribute("0", UIWidgets.Auto, desc: "Spawn Default Gunner", category: "Crew")]
	protected bool m_spawnTurret;
	[Attribute("0", UIWidgets.Auto, desc: "Spawn Default Passengers", category: "Crew")]
	protected bool m_spawnCargo;
	
	[Attribute("", UIWidgets.ResourceAssignArray, desc: "If defined, fill seats with these characters instead of the vehicles default characters. \nList of character prefabs, empty resources will become empty seats. \nYou can check the vehicles CompartmentManagerComponent for the primary compartments slot order, secondary compartsments (e. g. BTR gunner) come afterwards.", category: "Crew", params: "et")]
	protected ref array<ResourceName> m_customCrew;
	
	[Attribute("", UIWidgets.Auto, desc: "Names of existing waypoints to be assigned after delay", category: "Waypoints")]
	protected ref array<string> m_waypointNames;
	[Attribute("5", UIWidgets.Auto, desc: "After how many seconds to assign the waypoints. \nThis timer starts counting right after spawning, and does not wait for briefing end! \nIf you want a delay that starts at briefing end, use a mission event with a spawn vehicle instruction!", category: "Waypoints", params: "0 inf 0.1")]
	protected float m_waypointDelay;
	
	[Attribute("0", UIWidgets.Auto, desc: "If entity is a static turret, prevent the gunner from dismounting, even if unable to engage threats.", category: "Other")]
	protected bool m_noTurretDismount;
	
	[Attribute("0", UIWidgets.Auto, desc: "Put all gunners into a separate group, so that they remain idle (and can engage targets), while only driver + passengers follow the waypoint.", category: "Other")]
	protected bool m_idleGroup;
	
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		Managed m = owner.FindComponent(SCR_BaseCompartmentManagerComponent);
		if (!m) return;
		SCR_BaseCompartmentManagerComponent cm = SCR_BaseCompartmentManagerComponent.Cast(m);
		if (!cm) return;
		if (!GetGame().GetWorld()) return;
		GetGame().GetCallqueue().Call(AddCrew, cm);
	}
	
	protected void AddCrew(SCR_BaseCompartmentManagerComponent cm)
	{
		array<BaseCompartmentSlot> slots = {};
		// MIGHT NOT BE READY YET
		cm.GetCompartments(slots);
		
		array<ECompartmentType> cTypes = {};
		if (m_spawnPilot) cTypes.Insert(ECompartmentType.PILOT);
		if (m_spawnTurret) cTypes.Insert(ECompartmentType.TURRET);
		if (m_spawnCargo) cTypes.Insert(ECompartmentType.CARGO);
		
		GetGame().GetCallqueue().Call(SpawnCrew, slots, cTypes, m_customCrew, null, null, m_waypointNames, m_waypointDelay, m_noTurretDismount, m_idleGroup);
	}
	
	static void SpawnCrew(array<BaseCompartmentSlot> slots, array<ECompartmentType> cTypes, array<ResourceName> characters, AIGroup mainGroup, AIGroup idleGroup, array<string> waypointNames = null, float wpDelay = 0, bool noTurretDismount = false, bool useIdleGroup = false)
	{
		if (slots.IsEmpty() || !characters) // No more slots, or characters have been exhausted (and array has been set to null)
		{
			// Finished spawning characters
			GetGame().GetCallqueue().CallLater(AddWaypointsStatic, wpDelay * 1000, false, mainGroup, waypointNames);
			return;
		}
		
		// Spawn character
		IEntity ce;
		
		bool isValidType = cTypes.Contains(slots[0].GetType()); // is this slots type allowed
		if (!characters.IsEmpty() && characters[0] != "" && isValidType) { // character array not empty, use these
			// Custom character
			if (useIdleGroup && slots[0].GetType() == ECompartmentType.TURRET) ce = slots[0].SpawnCharacterInCompartment(characters[0], idleGroup);
			else ce = slots[0].SpawnCharacterInCompartment(characters[0], mainGroup);
		} else if (characters.IsEmpty() && isValidType) { // character array empty (but not null), use default characters
			// Default character
			if (useIdleGroup && slots[0].GetType() == ECompartmentType.TURRET) ce = slots[0].SpawnDefaultCharacterInCompartment(idleGroup);
			else ce = slots[0].SpawnDefaultCharacterInCompartment(mainGroup);
		}
		
		// Prevent turret dismount
		if (ce && noTurretDismount && slots[0].GetType() == ECompartmentType.TURRET) {
			Managed m = ce.FindComponent(SCR_AICombatComponent);
			if (m) {
				SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(m);
				cc.m_neverDismountTurret = true;
			}
		}
		
		// Remove from spawn list
		if (slots && !slots.IsEmpty()) slots.Remove(0);
		if (!characters.IsEmpty() && isValidType) { // if character from array was spawned, remove it
			characters.Remove(0);
			if (characters.IsEmpty()) characters = null; // if this was the last character, set to null as sign to stop
		}
		
		// Queue up next character
		GetGame().GetCallqueue().Call(SpawnCrew, slots, cTypes, characters, mainGroup, idleGroup, waypointNames, wpDelay, noTurretDismount, useIdleGroup);
	}
	
	static void AddWaypointsStatic(AIGroup g, array<string> waypointNames)
	{
		if (!g || !waypointNames || waypointNames.IsEmpty()) return;
		foreach (string name : waypointNames)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(name);
			if (!e) continue;
			AIWaypoint wp = AIWaypoint.Cast(e);
			if (!wp) continue;
			g.AddWaypoint(wp);
		}
	}
	
	static void PreventTurretDismount(SCR_BaseCompartmentManagerComponent cm)
	{
		array<IEntity> occupants = new array<IEntity>;
		cm.GetOccupantsOfType(occupants, ECompartmentType.TURRET);
		foreach(IEntity gunner : occupants)
		{
			Managed m = gunner.FindComponent(SCR_AICombatComponent);
			if (!m) continue;
			SCR_AICombatComponent combComp = SCR_AICombatComponent.Cast(m);
			combComp.m_neverDismountTurret = true;
		}
	}
}