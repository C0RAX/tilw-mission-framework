modded class SCR_AICombatComponent : ScriptComponent
{
	bool m_neverDismountTurret = false;

	override bool DismountTurretCondition(inout vector targetPos, bool targetPosProvided, out float threatPriority)
	{
		if (m_neverDismountTurret)
			return false;
		return super.DismountTurretCondition(targetPos, targetPosProvided, threatPriority);
	}
}
