#include "component/controller/helper/ActivationTranslator.h"

#include "data/access/StorageAccess.h"
#include "utility/messaging/type/MessageActivateAll.h"
#include "utility/messaging/type/MessageActivateEdge.h"
#include "utility/messaging/type/MessageActivateFile.h"
#include "utility/messaging/type/MessageActivateNodes.h"
#include "utility/messaging/type/MessageActivateTokenIds.h"
#include "utility/messaging/type/MessageActivateTokens.h"
#include "utility/messaging/type/MessageChangeFileView.h"
#include "utility/messaging/type/MessageSearch.h"
#include "utility/messaging/type/MessageShowErrors.h"

ActivationTranslator::ActivationTranslator(StorageAccess* storageAccess)
	: m_storageAccess(storageAccess)
{
}

ActivationTranslator::~ActivationTranslator()
{
}

std::shared_ptr<MessageActivateTokens> ActivationTranslator::translateMessage(const MessageActivateEdge* message) const
{
	std::shared_ptr<MessageActivateTokens> m;
	if (message->type == Edge::EDGE_AGGREGATION)
	{
		const Id sourceId = m_storageAccess->getIdForNodeWithNameHierarchy(message->fromNameHierarchy);
		const Id targetId = m_storageAccess->getIdForNodeWithNameHierarchy(message->toNameHierarchy);

		m = std::make_shared<MessageActivateTokens>(m_storageAccess->getTokenIdsForAggregationEdge(sourceId, targetId));
		m->isAggregation = true;
		m->undoRedoType = message->undoRedoType;
	}
	else
	{
		Id edgeId = message->tokenId;

		if (!message->isFresh())
		{
			edgeId = m_storageAccess->getIdForEdge(message->type, message->fromNameHierarchy, message->toNameHierarchy);
		}

		if (edgeId)
		{
			m = std::make_shared<MessageActivateTokens>(std::vector<Id>(1, edgeId));
			m->isEdge = true;
			m->undoRedoType = message->undoRedoType;
			m->setKeepContent(message->keepContent());
		}
	}
	return m;
}

std::shared_ptr<MessageActivateTokens> ActivationTranslator::translateMessage(const MessageActivateFile* message) const
{
	Id fileId = m_storageAccess->getTokenIdForFileNode(message->filePath);

	if (fileId == 0)
	{
		MessageChangeFileView msg(
			message->filePath,
			MessageChangeFileView::FILE_MAXIMIZED,
			true,
			false,
			nullptr
		);
		msg.undoRedoType = message->undoRedoType;
		msg.setKeepContent(message->keepContent());
		msg.dispatch();
		return nullptr;
	}

	std::shared_ptr<MessageActivateTokens> m;
	m = std::make_shared<MessageActivateTokens>(std::vector<Id>(1, fileId));
	m->undoRedoType = message->undoRedoType;
	m->setKeepContent(message->keepContent());
	return m;
}

std::shared_ptr<MessageActivateTokens> ActivationTranslator::translateMessage(const MessageActivateNodes* message) const
{
	std::vector<Id> nodeIds;
	if (message->isFresh())
	{
		for (const MessageActivateNodes::ActiveNode& node : message->nodes)
		{
			nodeIds.push_back(node.nodeId);
		}
	}
	else
	{
		for (const MessageActivateNodes::ActiveNode& node : message->nodes)
		{
			Id nodeId = m_storageAccess->getIdForNodeWithNameHierarchy(node.nameHierarchy);
			if (nodeId > 0)
			{
				nodeIds.push_back(nodeId);
			}
		}
	}

	std::shared_ptr<MessageActivateTokens> m;
	m = std::make_shared<MessageActivateTokens>(nodeIds);
	m->isFromSystem = message->isFromSystem;
	m->undoRedoType = message->undoRedoType;
	m->setKeepContent(message->keepContent());
	return m;
}

std::shared_ptr<MessageActivateTokens> ActivationTranslator::translateMessage(const MessageActivateTokenIds* message) const
{
	std::shared_ptr<MessageActivateTokens> m;
	m = std::make_shared<MessageActivateTokens>(message->tokenIds);
	m->undoRedoType = message->undoRedoType;
	m->setKeepContent(message->keepContent());
	return m;
}

std::shared_ptr<MessageActivateTokens> ActivationTranslator::translateMessage(const MessageSearch* message) const
{
	const std::vector<SearchMatch>& matches = message->getMatches();

	for (const SearchMatch& match : matches)
	{
		if (match.searchType == SearchMatch::SEARCH_COMMAND &&
			match.getFullName() == SearchMatch::getCommandName(SearchMatch::COMMAND_ALL))
		{
			MessageActivateAll msg;
			msg.undoRedoType = message->undoRedoType;
			msg.dispatchImmediately();
			return nullptr;
		}
		else if (match.searchType == SearchMatch::SEARCH_COMMAND &&
			match.getFullName() == SearchMatch::getCommandName(SearchMatch::COMMAND_ERROR))
		{
			MessageShowErrors msg(-1);
			msg.undoRedoType = message->undoRedoType;
			msg.dispatchImmediately();
			return nullptr;
		}
	}

	std::vector<Id> tokenIds = m_storageAccess->getTokenIdsForMatches(matches);
	tokenIds = m_storageAccess->getActiveTokenIdsForTokenIds(tokenIds);

	std::shared_ptr<MessageActivateTokens> m;
	m = std::make_shared<MessageActivateTokens>(tokenIds);
	m->undoRedoType = message->undoRedoType;
	m->setKeepContent(message->keepContent());
	if (message->isFresh())
	{
		m->isFromSearch = true;
	}
	return m;
}
