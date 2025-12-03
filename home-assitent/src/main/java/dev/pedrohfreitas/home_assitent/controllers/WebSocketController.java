package dev.pedrohfreitas.home_assitent.controllers;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.socket.WebSocketSession;

import dev.pedrohfreitas.home_assitent.records.DeviceInfo;
import dev.pedrohfreitas.home_assitent.services.WebSocketService;

@Controller
public class WebSocketController {
	
	private final WebSocketService webSocketService;
	
	public WebSocketController(WebSocketService webSocketService) {
		this.webSocketService = webSocketService;
	}
	
	@GetMapping("devices")
	@ResponseBody
	public List<DeviceInfo> devices() throws Exception {
		return webSocketService.sessions.entrySet().stream().map(item -> {
			return new DeviceInfo(item.getKey(), item.getValue().getRemoteAddress().getAddress().getAddress().toString());
		}).collect(Collectors.toList());
	}

	@PostMapping(path="sendMessage/{userId}")
	@ResponseBody
	public String sendSimpleMessage(@PathVariable("userId") String userId,
	        @RequestBody String message) throws Exception {
		webSocketService.enviarSomentePara(userId, message);
		return "ok";
	}
}
